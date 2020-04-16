#include <map>
#include <algorithm>

#include "ItemNotFoundException.h"
#include "MemoryAccess.h"



int getUsers(void* data, int argc, char** argv, char** colName);
int getPictures(void* data, int argc, char** argv, char** colName);
int getTags(void* data, int argc, char** argv, char** colName);

void MemoryAccess::printAlbums() 
{
	if(m_albums.empty()) {
		throw MyException("There are no existing albums.");
	}
	std::cout << "Album list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (auto a : m_albums) 	{
		std::cout <<  "* " << a << "creation data : " << a.getCreationDate() << "\n";
	}
}

/*
connect to db and create tables.
*/
bool MemoryAccess::open()
{
	std::string db_name = "db.sqlite";
	int file_not_exist = _access(db_name.c_str(), 0);
	int db_conection = sqlite3_open(db_name.c_str(), &db);

	if (db_conection != SQLITE_OK) 
	{
		std::cout << "failed to open db";
		return false;
	}
	if (file_not_exist) 
	{
		char* errMsg = "error";
		char* sql_query = "CREATE TABLE ALBUMS (NAME TEXT NOT NULL, CRT_DATE TEXT NOT NULL, USER_ID INTEGER NOT NULL);";
		int query_ok = sqlite3_exec(db, sql_query, nullptr, nullptr, &errMsg);
		if (query_ok != SQLITE_OK)
			throw MyException("coudent create albums table");
		
		sql_query = "CREATE TABLE PICTURES (ID INTEGER PRIMARY KEY NOT NULL, NAME TEXT NOT NULL, LOCATION TEXT NOT NULL, CRT_DATE TEXT NOT NULL , ALBUM_NAME TEXT NOT NULL , USER_ID INTEGER NOT NULL);"; 
		query_ok = sqlite3_exec(db, sql_query, nullptr, nullptr, &errMsg);
		if (query_ok != SQLITE_OK)
			throw MyException("coudent create pictures table");

		sql_query = "CREATE TABLE USERS (ID INTEGER PRIMARY KEY NOT NULL, NAME TEXT NOT NULL);";
		query_ok = sqlite3_exec(db, sql_query, nullptr, nullptr, &errMsg);
		if (query_ok != SQLITE_OK)
			throw MyException("coudent create users table");

		sql_query = "CREATE TABLE TAGS (USER_ID INTEGER NOT NULL , PICTURE_ID INTEGER NOT NULL , ALBUM_NAME TEXT NOT NULL , OWNER_ID INTEGER NOT NULL);";
		query_ok = sqlite3_exec(db, sql_query, nullptr, nullptr, &errMsg);
		if (query_ok != SQLITE_OK)
			throw MyException("coudent create tags table");

	}
	getDataFronDb();
	return true;

}

void MemoryAccess::clear()
{
	m_users.clear();
	m_albums.clear();
}

auto MemoryAccess::getAlbumIfExists(const std::string & albumName)
{
	auto result = std::find_if(std::begin(m_albums), std::end(m_albums), [&](auto& album) { return album.getName() == albumName; });

	if (result == std::end(m_albums)) {
		throw ItemNotFoundException("Album not exists: ", albumName);
	}
	return result;

}

Album MemoryAccess::createDummyAlbum(const User& user)
{
	std::stringstream name("Album_" +std::to_string(user.getId()));

	Album album(user.getId(),name.str());

	for (int i=1; i<3; ++i)	{
		std::stringstream picName("Picture_" + std::to_string(i));

		Picture pic(i++, picName.str());
		pic.setPath("C:\\Pictures\\" + picName.str() + ".bmp");

		album.addPicture(pic);
	}

	return album;
}

const std::list<Album> MemoryAccess::getAlbums() 
{
	return m_albums;
}

const std::list<Album> MemoryAccess::getAlbumsOfUser(const User& user) 
{	
	std::list<Album> albumsOfUser;
	for (const auto& album: m_albums) {
		if (album.getOwnerId() == user.getId()) {
			albumsOfUser.push_back(album);
		}
	}
	return albumsOfUser;
}
/*
inserting new value to albums
*/
void MemoryAccess::createAlbum(const Album& album)
{
	std::string sql_query = "INSERT INTO ALBUMS (NAME , CRT_DATE , USER_ID) VALUES (\""  + album.getName() + "\", \"" + album.getCreationDate() + "\" ," + std::to_string(album.getOwnerId()) + ");";
	execSqlQuery(sql_query);
	m_albums.push_back(album);
}
/*
deleting album from db and all related to him (pictures , tags)
*/
void MemoryAccess::deleteAlbum(const std::string& albumName, int userId)
{
	std::string sql_query = "DELETE FROM ALBUMS WHERE NAME ==\"" + albumName + "\" AND USER_ID ==" + std::to_string(userId) + ";";
	execSqlQuery(sql_query);
	sql_query = "DELETE FROM PICTURES WHERE ALBUM_NAME ==\"" + albumName + "\" AND USER_ID ==" + std::to_string(userId) + ";";
	execSqlQuery(sql_query);
	sql_query = "DELETE FROM TAGS WHERE ALBUM_NAME ==\"" + albumName + "\" AND USER_ID ==" + std::to_string(userId) + ";";
	execSqlQuery(sql_query);
	for (auto iter = m_albums.begin(); iter != m_albums.end(); iter++) {
		if ( iter->getName() == albumName && iter->getOwnerId() == userId ) {
			iter = m_albums.erase(iter);
			return;
		}
	}
}

bool MemoryAccess::doesAlbumExists(const std::string& albumName, int userId) 
{
	for (const auto& album: m_albums) {
		if ( (album.getName() == albumName) && (album.getOwnerId() == userId) ) {
			return true;
		}
	}

	return false;
}

Album MemoryAccess::openAlbum(const std::string& albumName) 
{
	for (auto& album: m_albums)	{
		if (albumName == album.getName()) {
			return album;
		}
	}
	throw MyException("No album with name " + albumName + " exists");
}
/*
inserting new picture into pictures
*/
void MemoryAccess::addPictureToAlbumByName(const std::string& albumName, const Picture& picture) 
{
	auto result = getAlbumIfExists(albumName);

	(*result).addPicture(picture);
	std::string sql_query = "INSERT INTO PICTURES (ID , NAME , LOCATION , CRT_DATE , ALBUM_NAME , USER_ID) VALUES (" + std::to_string(picture.getId()) + ",\"" + picture.getName() + "\" ,\"" + picture.getPath() + "\" ,\"" + picture.getCreationDate() + "\" ,\"" + albumName + "\"," + std::to_string((*result).getOwnerId()) + ");";
	execSqlQuery(sql_query);
}
/*
remove picture and her related tags
*/
void MemoryAccess::removePictureFromAlbumByName(const std::string& albumName, const std::string& pictureName) 
{
	auto result = getAlbumIfExists(albumName);
	std::string sql_query = "DELETE FROM PICTURES WHERE NAME == \"" + pictureName + "\" AND ALBUM_NAME ==\"" + albumName + "\";";
	execSqlQuery(sql_query);

	sql_query = "DELETE FROM TAGS WHERE PICTURE_ID ==" + std::to_string((*result).getPicture(pictureName).getId()) + ";";
	execSqlQuery(sql_query);
	(*result).removePicture(pictureName);
}
/*
inserting new row in tags table
*/
void MemoryAccess::tagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	auto result = getAlbumIfExists(albumName);
	(*result).tagUserInPicture(userId, pictureName);
	std::string sql_query = "INSERT INTO TAGS (USER_ID , PICTURE_ID , ALBUM_NAME , OWNER_ID) VALUES (" + std::to_string(userId) + "," + std::to_string((*result).getPicture(pictureName).getId()) + ", \"" + albumName + "\","  + std::to_string((*result).getOwnerId()) + ");";
	execSqlQuery(sql_query);
}
/*
deleting tag from db
*/
void MemoryAccess::untagUserInPicture(const std::string& albumName, const std::string& pictureName, int userId)
{
	auto result = getAlbumIfExists(albumName);
	std::string sql_query = "DELETE FROM TAGS WHERE USER_ID = " + std::to_string(userId) + " AND PICTURE ID = " + std::to_string((*result).getPicture(pictureName).getId()) + ";";
	execSqlQuery(sql_query);

	(*result).untagUserInPicture(userId, pictureName);
}

void MemoryAccess::closeAlbum(Album& ) 
{
	// basically here we would like to delete the allocated memory we got from openAlbum
}

// ******************* User ******************* 
void MemoryAccess::printUsers()
{
	std::cout << "Users list:" << std::endl;
	std::cout << "-----------" << std::endl;
	for (const auto& user: m_users) {
		std::cout << user << std::endl;
	}
}

User MemoryAccess::getUser(int userId) {
	for (const auto& user : m_users) {
		if (user.getId() == userId) {
			return user;
		}
	}

	throw ItemNotFoundException("User", userId);
}
/*
inserting new user into the db
*/
void MemoryAccess::createUser(User& user)
{
	std::string sql_query = "INSERT INTO USERS (ID , NAME) VALUES (" + std::to_string(user.getId()) + ", \"" + user.getName() + "\");";
	execSqlQuery(sql_query);
	m_users.push_back(user);
}
/*
delete user from db and all related to him
*/
void MemoryAccess::deleteUser(const User& user)
{
	if (doesUserExists(user.getId())) {
	
		for (auto iter = m_users.begin(); iter != m_users.end(); ++iter) {
			if (*iter == user) {
				m_users.erase(iter);
				break;
			}
		}
		auto iter = m_albums.begin();
		while (iter != m_albums.end()) {
			if (iter->getOwnerId() == user.getId()) {
				m_albums.erase(iter);
				iter = m_albums.begin();
			}
			else
				++iter;
		}
		std::string sql_query = "DELETE FROM USERS WHERE ID = " + std::to_string(user.getId()) + ";";
		execSqlQuery(sql_query);
		sql_query = "DELETE FROM ALBUMS WHERE USER_ID = " + std::to_string(user.getId()) + ";";
		execSqlQuery(sql_query);
		sql_query = "DELETE FROM PICTURES WHERE USER_ID = " + std::to_string(user.getId()) + ";";
		execSqlQuery(sql_query);
		sql_query = "DELETE FROM TAGS WHERE OWNER_ID = " + std::to_string(user.getId()) + " OR USER_ID = " + std::to_string(user.getId()) + ";";
		execSqlQuery(sql_query);
		return;
	}
}

bool MemoryAccess::doesUserExists(int userId) 
{
	auto iter = m_users.begin();
	for (const auto& user : m_users) {
		if (user.getId() == userId) {
			return true;
		}
	}
	
	return false;
}


// user statistics
int MemoryAccess::countAlbumsOwnedOfUser(const User& user) 
{
	int albumsCount = 0;

	for (const auto& album: m_albums) {
		if (album.getOwnerId() == user.getId()) {
			++albumsCount;
		}
	}

	return albumsCount;
}

int MemoryAccess::countAlbumsTaggedOfUser(const User& user) 
{
	int albumsCount = 0;

	for (const auto& album: m_albums) {
		const std::list<Picture>& pics = album.getPictures();
		
		for (const auto& picture: pics)	{
			if (picture.isUserTagged(user))	{
				albumsCount++;
				break;
			}
		}
	}

	return albumsCount;
}

int MemoryAccess::countTagsOfUser(const User& user) 
{
	int tagsCount = 0;

	for (const auto& album: m_albums) {
		const std::list<Picture>& pics = album.getPictures();
		
		for (const auto& picture: pics)	{
			if (picture.isUserTagged(user))	{
				tagsCount++;
			}
		}
	}

	return tagsCount;
}

float MemoryAccess::averageTagsPerAlbumOfUser(const User& user) 
{
	int albumsTaggedCount = countAlbumsTaggedOfUser(user);

	if ( 0 == albumsTaggedCount ) {
		return 0;
	}

	return static_cast<float>(countTagsOfUser(user)) / albumsTaggedCount;
}

User MemoryAccess::getTopTaggedUser()
{
	std::map<int, int> userTagsCountMap;

	auto albumsIter = m_albums.begin();
	for (const auto& album: m_albums) {
		for (const auto& picture: album.getPictures()) {
			
			const std::set<int>& userTags = picture.getUserTags();
			for (const auto& user: userTags ) {
				//As map creates default constructed values, 
				//users which we haven't yet encountered will start from 0
				userTagsCountMap[user]++;
			}
		}
	}

	if (userTagsCountMap.size() == 0) {
		throw MyException("There isn't any tagged user.");
	}

	int topTaggedUser = -1;
	int currentMax = -1;
	for (auto entry: userTagsCountMap) {
		if (entry.second < currentMax) {
			continue;
		}

		topTaggedUser = entry.first;
		currentMax = entry.second;
	}

	if ( -1 == topTaggedUser ) {
		throw MyException("Failed to find most tagged user");
	}

	return getUser(topTaggedUser);
}

Picture MemoryAccess::getTopTaggedPicture()
{
	int currentMax = -1;
	const Picture* mostTaggedPic = nullptr;
	for (const auto& album: m_albums) {
		for (const Picture& picture: album.getPictures()) {
			int tagsCount = picture.getTagsCount();
			if (tagsCount == 0) {
				continue;
			}

			if (tagsCount <= currentMax) {
				continue;
			}

			mostTaggedPic = &picture;
			currentMax = tagsCount;
		}
	}
	if ( nullptr == mostTaggedPic ) {
		throw MyException("There isn't any tagged picture.");
	}

	return *mostTaggedPic;	
}

std::list<Picture> MemoryAccess::getTaggedPicturesOfUser(const User& user)
{
	std::list<Picture> pictures;

	for (const auto& album: m_albums) {
		for (const auto& picture: album.getPictures()) {
			if (picture.isUserTagged(user)) {
				pictures.push_back(picture);
			}
		}
	}

	return pictures;
}
/*
executing query and rasing exception if failed
*/
void MemoryAccess :: execSqlQuery(std::string sql_query) 
{
	char* errMsg = "error";
	int query_ok = sqlite3_exec(db, sql_query.c_str(), nullptr, nullptr, &errMsg);
	if (query_ok != SQLITE_OK)
		throw MyException("something went wrong ");

}
/*
getting all data from db
*/
void MemoryAccess ::getDataFronDb()
{
	int getAlbums(void* data, int argc, char** argv, char** colName);
	char* errMsg = "error";
	std::string sql_query = "SELECT * FROM ALBUMS;";
	int query_ok = sqlite3_exec(db, sql_query.c_str(), getAlbums, (void*)&m_albums , &errMsg);
	sql_query = "SELECT * FROM USERS;";
	query_ok = sqlite3_exec(db, sql_query.c_str(), getUsers, (void*)&m_users, &errMsg);
	sql_query = "SELECT * FROM PICTURES;";
	query_ok = sqlite3_exec(db, sql_query.c_str(), getPictures , (void*)&m_albums, &errMsg);
	sql_query = "SELECT * FROM TAGS;";
	query_ok = sqlite3_exec(db, sql_query.c_str(), getTags, (void*)&m_albums, &errMsg);
}

/*
getting all existing albums and inset them into the album list
*/
int getAlbums(void* data, int argc, char** argv, char** colName)
{
	std::list<Album>* albums = (std::list<Album>*)data;
	for (int i = 0; i < argc; i+=3)
	{
		Album* alb = new Album();
		alb->setName(argv[i]);
		alb->setCreationDate(argv[i+1]);
		alb->setOwner(std::atoi(argv[i + 2]));
		albums->push_back(*alb);
	}
	return 0;
}
/*
getting all existing users and inset them into the album list
*/
int getUsers(void* data, int argc, char** argv, char** colName)
{
	std::list<User>* users = (std::list<User>*)data;
	for (int i = 0; i < argc; i += 2)
	{
		User* usr = new User(std::atoi(argv[i]) ,argv[i+1]);
		users->push_back(*usr);
	}
	return 0;
}
/*
getting all existing pictures and inset them into the album list
*/
int getPictures(void* data, int argc, char** argv, char** colName) 
{
	std::list<Album>* albums = (std::list<Album>*)data;
	for (int i = 0 ; i < argc ; i += 6) 
	{
		Picture* pic = new Picture(std::atoi(argv[i]), argv[i + 1], argv[i + 2], argv[i + 3]);
		std::list <Album> :: iterator it;
		for (it = albums->begin() ; it != albums->end() ; ++it) 
		{
			if (it->getName() == std::string(argv[i + 4]))
				it->addPicture(*pic);
		}
	}
	return 0;
}
/*
getting all existing tags and inset them into the album list
*/
int getTags(void* data, int argc, char** argv, char** colName) 
{
	std::list<Album>* albums = (std::list<Album>*)data;
	for (int i = 0 ; i < argc ; i+=4) 
	{
		std::list <Album> ::iterator it;
		for (it = albums->begin() ; it != albums->end() ; ++it) 
		{
			if (it->getName() == std::string(argv[i+2])) 
			{
				it->getPicture(std::atoi(argv[i + 1]))->tagUser(std::atoi(argv[i]));
			}
		}
	}
	return 0;
}
/*
getting highst user id
*/
int MemoryAccess :: getHighstUserId () 
{
	int highst = 0;
	std::list<User> ::iterator it;
	for (it = m_users.begin() ; it != m_users.end() ; ++it) 
	{
		if (it->getId() > highst)
			highst = it->getId();
	}
	return highst;
}
/*
getting highst picture id
*/
int MemoryAccess::getHighstPictureId() 
{
	int highst = 0;
	std::list<Album> ::iterator it;
	for (it = m_albums.begin() ; it != m_albums.end() ; ++it) 
	{
		std::list<Picture> :: iterator itr;
		std::list<Picture> pics = it->getPictures();
		for (itr = pics.begin(); itr != pics.end() ; ++itr) 
		{
			if (itr->getId() > highst)
				highst = itr->getId();
		}
	}
	return highst;
}
