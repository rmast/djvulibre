/***
 * 
 * Exportshapes is a program that extracts shape dictionary data 
 * from a djvu file into a database.
 *
 * Copyright © 2012 -- Piotr Sikora.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 * 
 ***/

#include <iostream>
#include <exception>
#include <string>
#include <sstream>
#include <map>
#include <vector>

#include <mysql.h>

#include <boost/scope_exit.hpp>

#include <DjVuDocument.h>
#include <DjVuImage.h>
#include <DjVmDir.h>
#include <JB2Image.h>
#include <Arrays.h>
#include <GBitmap.h>
#include <UnicodeByteStream.h>

using namespace std;

typedef std::pair<int, int> IntPair;

typedef struct {
	int right, top, left, bottom;
} BoundingBox;

bool test_run = false;

int create_database(const char * dbname, MYSQL *conn, const bool overwrite) {
	if (overwrite) {
		string query = "drop database if exists ";
		query.append(dbname);
		if (mysql_query(conn, query.c_str())) {
			std::cerr << "Dropping the database failed:  " << mysql_errno(conn) << ": " <<  mysql_error(conn) << std::endl;
		}
	}
	string query = "create database ";
	query.append(dbname);
	if (mysql_query(conn, query.c_str())) {
		std::cerr << "Error " << mysql_errno(conn) << ": " <<  mysql_error(conn) << std::endl;
		return 1;
	}
	return 0;
}

/***
 *
 * Create a table to link inherited dictionaries to pages in a document.
 * Outside create_tables method for backwards compatibility.
 *
 ***/
int create_pages_table(MYSQL *conn) {
	int retval;
	retval = mysql_query(conn,
		"create table if not exists pages (document_id INT not null, inh_dict_id INT not null, page_number INT not null);"
			);
	if (retval) {
		std::cerr << "Error during table creation:  " << mysql_errno(conn) << ": " <<  mysql_error(conn) << std::endl;
	}
	return retval;
}


int create_tables(MYSQL *conn) { // create tables
	if (mysql_query(conn,
		"create table shapes (id INT not null auto_increment primary key, original_id INT not null, parent_id INT not null, bits LONGBLOB, width INT, height INT, dictionary_id INT not null, bbox_top INT, bbox_left INT, bbox_right INT, bbox_bottom INT)"
			)){
		std::cerr << "Error during table creation: " << mysql_errno(conn) << ": " <<  mysql_error(conn) << std::endl;
		return 1;
	}
	if (mysql_query(conn,
		"create table blits (id INT not null auto_increment primary key, document_id INT not null, page_number INT not null, shape_id INT not null,	b_left SMALLINT UNSIGNED not null, b_bottom SMALLINT UNSIGNED not null);"
			)){
		std::cerr << "Error during table creation:  " << mysql_errno(conn) << ": " <<  mysql_error(conn) << std::endl;
		return 1;
	}
	if (mysql_query(conn,
		"create table documents (id INT not null auto_increment primary key, document varchar(60) not null, document_address varchar(100));"
			)){
		std::cerr << "Error during table creation:  " << mysql_errno(conn) << ": " <<  mysql_error(conn) << std::endl;
		return 1;
	}
	if (mysql_query(conn,
		"create table dictionaries (id INT not null auto_increment primary key, dictionary_name varchar(60) not null, page_number INT not null, document_id INT not null);"
			)){
		std::cerr << "Error during table creation:  " << mysql_errno(conn) << ": " <<  mysql_error(conn) << std::endl;
		return 1;
	}

	// -- page number for page-specific dictionaries, -1 for shared dictionaries

	return create_pages_table(conn);
}


BoundingBox compute_bounding_box(const GBitmap &bm)
{
	BoundingBox box;
	// Avoid trouble - TODO: not in a single threaded program? //  GMonitorLock lock(bm.monitor());
	// Get size
	const int w = bm.columns();
	const int h = bm.rows();
	const int s = bm.rowsize();
	// Right border
	int right, top, left, bottom;
	for(right=w-1;right >= 0;--right) {
		unsigned char const *p = bm[0] + right;
		unsigned char const * const pe = p+(s*h);
		for (;(p<pe)&&(!*p);p+=s)
			continue;
		if (p<pe)
			break;
	}
	box.right = right;
	// Top border
	for(top=h-1;top >= 0;--top)
    {
		unsigned char const *p = bm[top];
		unsigned char const * const pe = p+w;
		for (;(p<pe)&&(!*p); ++p)
			continue;
		if (p<pe)
			break;
	}
	box.top = top;
	// Left border
	for (left=0;left <= right;++left)
    {
		unsigned char const *p = bm[0] + left;
		unsigned char const * const pe=p+(s*h);
		for (;(p<pe)&&(!*p);p+=s)
			continue;
		if (p<pe)
			break;
    }
	box.left = left;
	// Bottom border
	for(bottom=0;bottom <= top;++bottom)
	{
		unsigned char const *p = bm[bottom];
		unsigned char const * const pe = p+w;
		for (;(p<pe)&&(!*p); ++p)
			continue;
		if (p<pe)
			break;
	}
	box.bottom = bottom;
	return box;
}

int test_blit = 0;

/***
 *
 * store_blit stores a blit in the database
 * and returns it's autogenerated database index.
 *
 *
 */
int store_blit(int shape_id, int doc_id, int page_number, unsigned short b_left, unsigned short b_bottom, MYSQL *conn) {
	if (test_run)
		return test_blit++;
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(conn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO blits(shape_id, b_left, b_bottom, page_number, document_id) VALUES (?,?,?,?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[5];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &shape_id;

    bind_structs[1].buffer_type = MYSQL_TYPE_SHORT;
    bind_structs[1].is_unsigned = 1;
    bind_structs[1].is_null_value = 0;
    bind_structs[1].buffer = (void *) &b_left;

    bind_structs[2].buffer_type = MYSQL_TYPE_SHORT;
    bind_structs[2].is_unsigned = 1;
    bind_structs[2].is_null_value = 0;
    bind_structs[2].buffer = (void *) &b_bottom;

    bind_structs[3].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[3].is_unsigned = 1;
    bind_structs[3].is_null_value = 0;
    bind_structs[3].buffer = (void *) &page_number;

    bind_structs[4].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[4].is_null_value = 0;
    bind_structs[4].buffer = (void *) &doc_id;

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    return mysql_insert_id(conn);
}


/***
 *
 * store_shape stores a shape in the database
 * and returns it's autogenerated database index.
 *

int new_store_shape(int original_id, GP<GBitmap> bits, int dictionary_id, int parent, MYSQL *conn) {
	char stmt_buf[MAX_IMAGE_SIZE*2+300], *p;
	//size_t length;
	int  status;

    GP<ByteStream> bs = ByteStream::create();
    BoundingBox bbox;


    if (bits) {
        bits->save_pbm(*bs);
        bbox = compute_bounding_box(*(bits));
    }

    sprintf (stmt_buf,
    		"INSERT INTO shapes(parent_id, dictionary_id, bbox_top, bbox_left, bbox_right, bbox_bottom, width, height, original_id, bits) VALUES (%d,%d,%d,%d,%d,%d,%d,%d,%d,'",
            parent, dictionary_id, bbox.top, bbox.left, bbox.right, bbox.bottom, bits->columns(), bits->rows(), original_id
            );
    p = stmt_buf + strlen (stmt_buf);

    // copy bits to the statement buffer
    size_t bytes_to_send = bs->size();

    char* data = (char*)bs->get_data();
    if (p + (2 * bytes_to_send) + 3 > stmt_buf + sizeof(stmt_buf)) {
    	std::cerr << "Shape image too big: " << bytes_to_send << std::endl;
    	return -1;
    }
    p += mysql_real_escape_string (conn, p, data, bytes_to_send);

    *p++ = '\'';
    *p++ = ')';
    status = mysql_real_query (conn, stmt_buf, (unsigned long) (p - stmt_buf));
    if (status != 0) {
    	std::cerr<< "Insert shape failed. " << std::endl;
    	return -1;
    }
    return mysql_insert_id(conn);
}*/

int store_shape(int original_id, GP<GBitmap> bits, int dictionary_id, int parent, MYSQL *conn) {
    GP<ByteStream> bs = ByteStream::create();
    BoundingBox bbox;
    if (bits) {
        bits->save_pbm(*bs);
        bbox = compute_bounding_box(*(bits));
    }

    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(conn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO shapes(parent_id, bits, dictionary_id, bbox_top, bbox_left, bbox_right, bbox_bottom, width, height, original_id) VALUES (?,?,?,?,?,?,?,?,?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[10];
    memset(bind_structs, 0, sizeof(bind_structs));

    // parent_id

    bind_structs[0].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &parent;

    // bits
    unsigned long length0;
    bind_structs[1].length = &length0;
    bind_structs[1].buffer_type = MYSQL_TYPE_BLOB;
    bind_structs[1].is_unsigned = 1;
    bind_structs[1].is_null_value = 0;

    // page_id
    bind_structs[2].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[2].is_null_value = 0;
    bind_structs[2].buffer = (void *) &dictionary_id;

    // bounding box, width, height

    int values[6];
    if (bits) {
    	values[0] = bbox.top;
    	values[1] = bbox.left;
    	values[2] = bbox.right;
    	values[3] = bbox.bottom;
    	values[4] = bits->columns(); // width
    	values[5] = bits->rows(); // height

    };

    for (int i = 0; i<6; i++) {
    	bind_structs[i+3].buffer_type = MYSQL_TYPE_LONG;
    	bind_structs[i+3].is_null_value = 0;
    	bind_structs[i+3].buffer = (void *) &values[i];
    }

    // original_id of the shape in the dictionary it comes from
    bind_structs[9].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[9].is_null_value = 0;
    bind_structs[9].buffer = (void *) &original_id;

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

	//    cout << "Bytes to read: " << bytes_to_read << endl;
	    //TODO: create a ByteStream subclass that will be a safe alternative to this (get_data guaranteed to work on ByteStream::Memorys only.

    size_t bytes_to_read = bs->size();
    size_t bytes_to_send = bytes_to_read;

    // need to allocate memory on the heap instead of stack in case an image is too big
    char* data = new char[bytes_to_read];
    // get the image data to the allocated buffer
    memcpy(data,(const char*) bs->get_data(), bytes_to_read);

    char * data_ptr = data;
    // send long images in 10kB chunks.
    const unsigned int chunk_size = 10*1024;

    char chunk[chunk_size];

    while (bytes_to_send > 0) {
    	size_t chunk_bytes = bytes_to_send > chunk_size ? chunk_size : bytes_to_send;
    	bytes_to_send -= chunk_bytes;
    	memcpy(chunk,data_ptr,chunk_bytes);
    	data_ptr += chunk_bytes;
    	if (mysql_stmt_send_long_data(pinsert_stmt, 1, chunk, chunk_bytes) != 0) {
    			   cerr << "Error: mysql_stmt_send_long_data() failed." << endl;
    			   delete data;
    			   return EXIT_FAILURE;
    	}
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        delete data;
        return EXIT_FAILURE;
    }

    delete data;
    return mysql_insert_id(conn);
}

int store_dictionary(int document_id, int page_number, string dict_name, MYSQL *conn) {
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(conn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO dictionaries(document_id, page_number, dictionary_name) VALUES (?,?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    const char * original_string = dict_name.c_str();
    unsigned long length = 2 * strlen (original_string) + 1;
    char prepared_string[length];
    unsigned long prepared_length = mysql_real_escape_string(conn, prepared_string, original_string, strlen(original_string));

    MYSQL_BIND bind_structs[3];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[0].is_unsigned = 1;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &document_id;

    bind_structs[1].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[1].is_unsigned = 1;
    bind_structs[1].is_null_value = 0;
    bind_structs[1].buffer = (void *) &page_number;

    bind_structs[2].length = &prepared_length;
    bind_structs[2].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind_structs[2].is_null = 0;
    bind_structs[2].buffer = (void *) &prepared_string;
    bind_structs[2].buffer_length = strlen(prepared_string) * sizeof(char);

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    return mysql_insert_id(conn);
}


int store_document (char * document, char * document_address, MYSQL *conn) {
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(conn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO documents(document, document_address) VALUES (?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }
    unsigned long length = 2 * strlen (document) + 1;
    char prepared_document[length];
    unsigned long prepared_length = mysql_real_escape_string(conn, prepared_document, document, strlen(document));

    unsigned long length_address = 2 * strlen (document_address) + 1;
    char prepared_document_address[length_address];
    unsigned long prepared_length_address = mysql_real_escape_string(conn, prepared_document_address, document_address, strlen(document_address));

    MYSQL_BIND bind_structs[2];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].length = &prepared_length;
    bind_structs[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind_structs[0].is_null = 0;
    bind_structs[0].buffer = (void *) &prepared_document;
    bind_structs[0].buffer_length = strlen(prepared_document) * sizeof(char);

    bind_structs[1].length = &prepared_length_address;
    bind_structs[1].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind_structs[1].is_null = 0;
    bind_structs[1].buffer = (void *) &prepared_document_address;
    bind_structs[1].buffer_length = strlen(prepared_document_address) * sizeof(char);

    cout << (char *) bind_structs[0].buffer << endl;
    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    return mysql_insert_id(conn);
}

int store_page_link (int document_id, int inh_dict_id, int page_number, MYSQL *conn) {
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(conn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO pages(document_id, inh_dict_id, page_number) VALUES (?,?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[3];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[0].is_unsigned = 1;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &document_id;

    bind_structs[1].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[1].is_unsigned = 1;
    bind_structs[1].is_null_value = 0;
    bind_structs[1].buffer = (void *) &inh_dict_id;

    bind_structs[2].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[2].is_unsigned = 1;
    bind_structs[2].is_null_value = 0;
    bind_structs[2].buffer = (void *) &page_number;

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    return 0;
}


int check_for_dictionary_in_database(std::string dictionary_name, MYSQL* conn) {
	if (test_run)
		std::cout << "Checking for dictionary: " << dictionary_name << std::endl;

	MYSQL_RES *result;
	MYSQL_ROW row;

	std::string query("SELECT id FROM dictionaries WHERE dictionary_name = \'");
	query += dictionary_name + '\'';

	if (test_run)
		std::cout << "Querying " << query << std::endl;

	int query_retval = mysql_query(conn, query.c_str());
	if (test_run)
		std::cout << "Queried with return code " << query_retval << mysql_error(conn) << std::endl;

	result = mysql_store_result(conn);

	if (test_run)
		std::cout << "Received result with " << mysql_num_rows(result) << " rows." << std::endl;

	if (mysql_num_rows(result) > 0) {
		row = mysql_fetch_row(result);
		if (test_run)
				std::cout << "Fetched a row " << row[0] << std::endl;
		mysql_free_result(result);
		return atoi(row[0]);
	}
	if (test_run)
			std::cout << "Freeing " << std::endl;
	mysql_free_result(result);
	return -1; // dictionary not found
}

int check_for_document_in_database(std::string document_name, MYSQL* conn) {
	if (test_run)
		std::cout << "Checking for document: " << document_name << std::endl;

	MYSQL_RES *result;
	MYSQL_ROW row;

	std::string query("SELECT id FROM documents WHERE document = \'");
	query += document_name + '\'';

	if (test_run)
		std::cout << "Querying " << query << std::endl;

	int query_retval = mysql_query(conn, query.c_str());
	if (test_run)
		std::cout << "Queried with return code " << query_retval << mysql_error(conn) << std::endl;

	result = mysql_store_result(conn);

	if (test_run)
		std::cout << "Received result with " << mysql_num_rows(result) << " rows." << std::endl;

	if (mysql_num_rows(result) > 0) {
		row = mysql_fetch_row(result);
		if (test_run)
				std::cout << "Fetched a row " << row[0] << std::endl;
		mysql_free_result(result);
		return atoi(row[0]);
	}
	if (test_run)
			std::cout << "Freeing " << std::endl;
	mysql_free_result(result);
	return -1; // dictionary not found

}


void fetch_inherited_dictionary(int inh_dict_id, std::map<int,int> &inherited_shape_translation, MYSQL* conn) {
	if (test_run)
		std::cout << "Fetching inherited dictionary " << inh_dict_id << std::endl;
	MYSQL_RES *result;

	MYSQL_ROW row;
	std::string dict_id_str;
	std::stringstream convert;
	convert << inh_dict_id;
	dict_id_str = convert.str();

	std::string query("SELECT original_id, id FROM shapes WHERE dictionary_id = ");
	query += dict_id_str;

	mysql_query(conn, query.c_str());
	result = mysql_store_result(conn);
	while ((row = mysql_fetch_row(result))) {
		// row[0] - original_id;  row[1] - db id
		inherited_shape_translation.insert(IntPair(atoi(row[0]),atoi(row[1])));
	}
	mysql_free_result(result);
}


int process_document(int page_from, int page_to, GP<DjVuDocument> doc, int doc_id, bool links_only, MYSQL* conn) {
	const int pages = doc->get_pages_num();
	int page_start, page_limit;
	page_start = (page_from < 1) ? 0 : page_from - 1;
	if (page_to < 1 || page_to >= pages)
		page_limit = pages;
	else
		page_limit = page_to;
	if (page_limit <= page_start) {
		std::cout << "Can't process less than one page." << std::endl;
		return EXIT_FAILURE;
	}


	//std::map<string, int> inherited_dictionary_translation;
	//std::map<IntPair, int> inherited_shape_translation; // Dictionary x shape number -> shape_id

	// in most, if not all cases, all the pages using an inherited dictionary will be right next to each other
	// therefore, it's better for the program's performance to keep only the last used inherited dictionary in memory
	// whenever a dictionary shared by a new djvu page is different, we need to check if it is one already in the database:
	// if yes, load it from database. If not, put it there and make it the current dictionary.
	// Incidentally, loading a dictionary from database is also needed for the functionality of starting the processing from
	// a chosen page.

	std::string current_inherited_dictionary = "";
	std::map<int, int> inherited_shape_translation;
	int inh_dict_id;
	for(int page_number = page_start; page_number < page_limit; page_number++) {
		GP<DjVuImage> djvu_page = doc->get_page(page_number);
		if (!djvu_page) {
			std::cout << "Failed to get DjVuImage for page" << page_number << std::endl;
		} else {
			GP<DjVuFile> djvu_file = djvu_page->get_djvu_file();
			GP<JB2Image> jimg = djvu_page->get_fgjb();
			if (!djvu_file) {
				std::cout << "Failed to get DjVUFile for page " << page_number << std::endl;
			} else if (!jimg) {
				std::cout << "Failed to get JB2Image for page " << page_number << std::endl;
			} else {//export shapes
				std::cout << "Processing page " << page_number << " containing " << jimg->get_shape_count() << " shapes, " <<
						jimg->get_inherited_shape_count() << " of them inherited." << std::endl;

				GP<JB2Dict> inherited_dictionary = jimg->get_inherited_dict();
				
				int inh_sh_count = 0;

				if (inherited_dictionary) {
					if (test_run)
						std::cout << "Page has inherited dictionary." << std::endl;

					// list included files
					GPList<DjVuFile> included_files = djvu_file->get_included_files();
					if (included_files.size() > 0) {
						// find an included file corresponding to the inherited dictionary
						for (GPosition i = included_files ; i; ++i) {
							GP<DjVuFile> included_file = included_files[i];
							//get the dictionary name
							string dict_name = (string) included_file->get_url().fname();
							// check if it is a previously known inherited dictionary

							//map<string,int>::iterator contains = inherited_dictionary_translation.find(dict_name);
							//if (contains!=inherited_dictionary_translation.end()) {
								//inh_dict_id = contains->second;
							//} else {
								//std::cout << "Found a new included file: " << dict_name << endl;
							if (included_file->fgjd) { // this is an inherited dictionary
								// check if it is NOT a currently used dictionary
								// if it is, nothing to be done here
								if (dict_name.compare(current_inherited_dictionary)!=0) {
									inherited_shape_translation.clear(); // reset the map
									inh_dict_id = check_for_dictionary_in_database(dict_name, conn); // returns the id of the dictionary or -1 if not found
									if (test_run)
										std::cout << "Dictionary check result: " << inh_dict_id << std::endl;
									if (inh_dict_id >= 0) {
										if  (!links_only) {
											fetch_inherited_dictionary(inh_dict_id, inherited_shape_translation, conn); // fills the map with shape id translation from the database
											if (test_run)
												std::cout << "Fetched inherited dictionary." << std::endl;
										}
									} else {// a new inherited dictionary
										if (links_only) {
												std::cout << "File can't be processed with option '-l' ('links only') - it contains inherited dictionaries that aren't in the database." << endl;
												return EXIT_FAILURE;
										}
										std::cout << "Found a new inherited dictionary:" << dict_name << " containing "  << included_file->fgjd->get_shape_count() << " shapes." << endl;
										// store the inherited dictionary
										inh_dict_id = store_dictionary(doc_id, -1, dict_name, conn);
										// remember the dictionary
										//	inherited_dictionary_translation.insert(std::pair<string,int>(dict_name, inh_dict_id));
										current_inherited_dictionary = dict_name;
										// store inherited shapes
										inh_sh_count = jimg->get_inherited_shape_count();
										for (int i = 0; i < inh_sh_count; i++) {
											JB2Shape shape = jimg->get_shape(i);
//											int parent = shape.parent >= 0 ? inherited_shape_translation[IntPair(inh_dict_id, shape.parent)] : shape.parent;
											int parent = shape.parent >= 0 ? inherited_shape_translation[shape.parent] : shape.parent;
											int shape_id = store_shape(i, shape.bits, inh_dict_id, parent ,conn);
	//										inherited_shape_translation.insert(std::pair<IntPair,int>(IntPair(inh_dict_id, i),shape_id));
											inherited_shape_translation.insert(IntPair(i,shape_id));
										}
									}
									
									
									
								}
								store_page_link(doc_id, inh_dict_id, page_number, conn);
							}
						}
					}
				}
				if (!links_only) {
					//compute page dictionary parameters
					std::map<int, int> page_shape_translation;

					string page_name = (string) djvu_file->get_url().fname();
					int sh_count = jimg->get_shape_count();
					int blit_count = jimg->get_blit_count();
					// store page dictionary
					int page_dict_id = store_dictionary(doc_id,page_number,page_name,conn);

					if (test_run)
						std::cout << "Processing page shapes. " << std::endl;

					// store shapes
					for (int i=inh_sh_count; i < sh_count;i++) {
						JB2Shape shape = jimg->get_shape(i);
						int parent;
						if (shape.parent < 0) {
							parent = shape.parent;
						} else if (shape.parent < inh_sh_count) { // inherited parent
							parent = test_run ? 0 : inherited_shape_translation[shape.parent];
						} else { // other parent
							parent = page_shape_translation[shape.parent];
						}
						int shape_id = store_shape(i, shape.bits, page_dict_id, parent, conn);
						page_shape_translation.insert(IntPair(i,shape_id));
					}

					if (test_run)
						std::cout << "Processing blits. " << std::endl;
					// store blits
					for (int i = 0; i < blit_count; i++) {
						JB2Blit *blit = jimg->get_blit(i);
						if (blit) {
							int shape_id = -1;
							if ((int) blit->shapeno < inh_sh_count) {
								// shape from inherited dictionary
								shape_id = inherited_shape_translation[blit->shapeno];
							} else {
								// shape from page dictionary
								shape_id = page_shape_translation[blit->shapeno];
							}
							store_blit(shape_id, doc_id, page_number, blit->left, blit->bottom, conn);
						}
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}




void usage(char **argv) {
	std::cout << "Usage: " << argv[0] << "[-c | -i | -o] [-t <page #>] [-f <page #>] -h <host> -d <database> -u <user> -p <password> <filename>" << std::endl;
	std::cout << "Option -i creates tables filled by the program in the database. " << std::endl;
	std::cout << "Option -c creates the database (with required tables)." << std::endl;
	std::cout << "Option -o overwrites the database (if it already exists). If not, works just like -c." << std::endl;
	std::cout << "Option -f: its argument specifies a page number from which the processing should start." << std::endl;
	std::cout << "Option -t: its argument specifies a limit to the number of pages processed by the program." << std::endl;
}

int main(int argc, char **argv) {
	try {
		const char *db_name;
		const char *db_user;
		const char *db_host;
		const char *db_passwd;
		char *filename, *doc_address;
		int c;
		bool create_db = false, inject_db = false, overwrite_db = false, given_address = false, links_only = false;
		int required_arguments = 0;
		int page_from = 1 , page_to = -1;
		while ((c = getopt (argc, argv, "Tolicd:u:h:p:f:t:a:")) != -1)
			switch (c)
		    {
				case 'T':
					test_run = true;
					break;
				case 'd':
					db_name = optarg;
					required_arguments++;
					break;
				case 'u':
					db_user = optarg;
					required_arguments++;
					break;
				case 'h':
					db_host = optarg;
					required_arguments++;
					break;
				case 'p':
					db_passwd = optarg;
					required_arguments++;
					break;
				case 'f':
					page_from = atoi(optarg);
					break;
				case 't':
					page_to = atoi(optarg);
					break;
				case 'c':
					create_db = true;
					inject_db = true;
					break;
				case 'i':
					inject_db = true;
					break;
				case 'o':
					create_db = true;
					inject_db = true;
					overwrite_db = true;
					break;
				case 'a':
					doc_address = optarg;
					given_address = true;
					break;
				case 'l':
					links_only = true;
					break;
				case '?':
		    	 usage(argv);
		         return 1;
		       default:
		         usage(argv);
		         return 1;
		    }

		if (optind == argc -1) {
			filename = argv[optind];
			required_arguments++;
		}

		if (required_arguments != 5 ) {
			usage(argv);
			return 1;
		}
		
		if (!given_address) {
			doc_address = filename;
		}
		
		MYSQL *conn = mysql_init(NULL);
		BOOST_SCOPE_EXIT( (conn) ) {
			mysql_close(conn);
		} BOOST_SCOPE_EXIT_END

		const char * initial_db_name = create_db ? NULL : db_name;

		if (!mysql_real_connect(conn, db_host, db_user, db_passwd, initial_db_name, 0, NULL, CLIENT_COMPRESS)) {
		        cerr << "Error: mysql_real_connect() failed to connect to `" << db_name << "`: " << mysql_error(conn) << std::endl;
		        return EXIT_FAILURE;
		    }

		if (create_db) {
			int retval = create_database(db_name, conn, overwrite_db);
			if (retval != 0) {
				std::cerr << "Database could not be created, cancelling execution." << endl;
				return retval;
			} else { // change to newly created database
				if (mysql_change_user(conn, db_user, db_passwd, db_name)) {
					std::cerr << "Failed to change user.  Error: " << mysql_error(conn) << std::endl;
					return retval;
				}

			}
		}

		if (inject_db) {
			if ( create_tables(conn) != 0) {
				std::cerr << "Database schema could not be injected, cancelling further execution. " << endl;
				return EXIT_FAILURE;
			}
		}
		
		// create table 'pages' if it doesn't exist. For backwards compatibility.
		create_pages_table(conn);

		const GURL::Filename::UTF8 url(filename);


		GP<DjVuDocument> doc = DjVuDocument::create_wait(url);

		if (!doc) {
			std::cerr << "cannot open file" << std::endl;
			return -1;
		}

		int doc_id = check_for_document_in_database(filename, conn);
		if (doc_id == -1) {
			doc_id = store_document(filename, doc_address, conn);
		}
		return process_document(page_from, page_to, doc, doc_id, links_only, conn);
	}
	catch (exception& ex) {
		std::cout << "An exception occurred: " << ex.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

