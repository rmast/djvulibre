/*
 * export shapes
 *
 *  Created on: Jan 16, 2012
 *      Author: Piotr Sikora
 */

#include <iostream>
#include <exception>
#include <string>
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
#include <BSByteStream.h>

using namespace std;

typedef std::pair<int, int> IntPair;

typedef struct {
	int right, top, left, bottom;
} BoundingBox;

const char * get_schema_filename() {
	return "shape.sql";
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


/***
 *
 * store_blit stores a blit in the database
 * and returns it's autogenerated database index.
 *
 *
 */
int store_blit(int shape_id, int doc_id, int page_number, unsigned short b_left, unsigned short b_bottom, MYSQL *conn) {
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
 */
int store_shape(GP<GBitmap> bits, int dictionary_id, int parent, MYSQL *conn) {
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

    const char sql[] = "INSERT INTO shapes(parent_id, bits, dictionary_id, bbox_top, bbox_left, bbox_right, bbox_bottom) VALUES (?,?,?,?,?,?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[7];
    memset(bind_structs, 0, sizeof(bind_structs));

    // parent_id

    bind_structs[0].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &parent;

    // bits
    unsigned long length0;
    bind_structs[1].length = &length0;
    bind_structs[1].buffer_type = MYSQL_TYPE_BLOB;
    bind_structs[1].is_null_value = 0;

    // page_id
    bind_structs[2].buffer_type = MYSQL_TYPE_LONG;
    bind_structs[2].is_null_value = 0;
    bind_structs[2].buffer = (void *) &dictionary_id;

    // bounding box
 //   my_bool bb_null[4];
    int bbox_values[4];
    if (bits) {
    	bbox_values[0] = bbox.top;
    	bbox_values[1] = bbox.left;
    	bbox_values[2] = bbox.right;
    	bbox_values[3] = bbox.bottom;
    };

    for (int i = 0; i<4; i++) {
    	bind_structs[i+3].buffer_type = MYSQL_TYPE_LONG;
   // 	bind_structs[i+3].is_null = &bb_null[i];
    	bind_structs[i+3].is_null_value = 0;
    	bind_structs[i+3].buffer = (void *) &bbox_values[i];
    }

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    size_t bytes_to_read = bs->size();
//    cout << "Bytes to read: " << bytes_to_read << endl;
    //TODO: create a ByteStream subclass that will be a safe alternative to this (get_data guaranteed to work on ByteStream::Memorys only.
    char* data = (char*)bs->get_data();
    /** char buf[1024];
    bool finished = false;
    while (finished) {
           size_t res = bs->readall(buf,1024);
           if (res == 0) {
        	   finished = true;
           } else {
        	   cout << "Bytes read: " << res << endl;
           }
     } **/
    if (mysql_stmt_send_long_data(pinsert_stmt, 1, data, bytes_to_read) != 0) {
		   cerr << "Error: mysql_stmt_send_long_data() failed." << endl;
		   return EXIT_FAILURE;
	}

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }
//    cout << "Inserted record #" << mysql_insert_id(conn) << endl;
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


int process_document(GP<DjVuDocument> doc, int doc_id, MYSQL* conn) {
	const int pages = doc->get_pages_num();
	std::map<string, int> inherited_dictionary_translation;
	std::map<IntPair, int> inherited_shape_translation; // Dictionary x shape number -> shape_id

	for(int page_number = 0; page_number < pages; page_number++) {

		GP<DjVuImage> djvu_page = doc->get_page(page_number);

		if (!djvu_page) {
			std::cout << "get_page failed";
		} else {

			GP<DjVuFile> djvu_file = djvu_page->get_djvu_file();
			GP<JB2Image> jimg = djvu_page->get_fgjb();
			if (!djvu_file) {
				std::cout << "get_djvu_file failed";
			} else if (!jimg) {
				std::cout << "get_fgjb failed";
			} else {//export shapes
				std::cout << "Processing page " << page_number << " containing " << jimg->get_shape_count() << " shapes, " <<
						jimg->get_inherited_shape_count() << " of them inherited." << std::endl;
				GP<JB2Dict> inherited_dictionary = jimg->get_inherited_dict();
				int inh_dict_id = -1;
				int inh_sh_count = 0;
				if (inherited_dictionary) {
					// list included files
					GPList<DjVuFile> included_files = djvu_file->get_included_files();
					if (included_files.size() > 0) {
						// find an included file corresponding to the inherited dictionary
						for (GPosition i = included_files ; i; ++i) {
							GP<DjVuFile> included_file = included_files[i];
							//get the dictionary name

							string dict_name = (string) included_file->get_url().fname();
							// check if it is a previously known inherited dictionary
							map<string,int>::iterator contains = inherited_dictionary_translation.find(dict_name);
							if (contains!=inherited_dictionary_translation.end()) {
								inh_dict_id = contains->second;
							} else {
								std::cout << "Found a new included file: " << dict_name << endl;
								if (included_file->fgjd) {
									std::cout << " It's an inherited dictionary containing "  << included_file->fgjd->get_shape_count() << " shapes." << endl;
									// store the inherited dictionary
									inh_dict_id = store_dictionary(doc_id, -1, dict_name, conn);
									// remember the dictionary
									inherited_dictionary_translation.insert(std::pair<string,int>(dict_name, inh_dict_id));
									// store inherited shapes
									inh_sh_count = jimg->get_inherited_shape_count();
									for (int i = 0; i < inh_sh_count; i++) {
										JB2Shape shape = jimg->get_shape(i);
										int parent = shape.parent >= 0 ? inherited_shape_translation[IntPair(inh_dict_id, shape.parent)] : shape.parent;
										int shape_id = store_shape(shape.bits, inh_dict_id, parent ,conn);
										inherited_shape_translation.insert(std::pair<IntPair,int>(IntPair(inh_dict_id, i),shape_id));
									}
								}
							}
						}
					}
				}
				//compute page dictionary parameters
				std::map<int, int> page_shape_translation;
				string page_name = (string) djvu_file->get_url().fname();
				int sh_count = jimg->get_shape_count();
				int blit_count = jimg->get_blit_count();
				// store page dictionary
				int page_dict_id = store_dictionary(doc_id,page_number,page_name,conn);

				// store shapes
				for (int i=inh_sh_count; i < sh_count;i++) {
					JB2Shape shape = jimg->get_shape(i);
					int parent;
					if (shape.parent < 0) {
						parent = shape.parent;
					} else if (shape.parent < inh_sh_count) { // inherited parent
						parent = inherited_shape_translation[IntPair(inh_dict_id,shape.parent)];
					} else { // other parent
						parent = page_shape_translation[shape.parent];
					}
					int shape_id = store_shape(shape.bits, page_dict_id, parent ,conn);
					page_shape_translation.insert(IntPair(i,shape_id));
				}

				// store blits
			    for (int i = 0; i < blit_count; i++) {
			    	JB2Blit *blit = jimg->get_blit(i);
			    	if (blit) {
			    		int shape_id = -1;
			    		if ((int) blit->shapeno < inh_sh_count) {
			    			// shape from inherited dictionary
			    			shape_id = inherited_shape_translation[IntPair(inh_dict_id, blit->shapeno)];
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
	return EXIT_SUCCESS;
}




void usage(char **argv) {
	std::cout << "Usage: " << argv[0] << " -h <host> -d <database> -u <user> -p <password> <filename>" << std::endl;
	//std::cout << "Option -i injects tables required by exportshapes into the database. " << std::endl;
	//std::cout << "Option -c creates the database (with required tables)." << std::endl;
}

int store_document (char * document, MYSQL *conn) {
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(conn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO documents(document) VALUES (?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }
    unsigned long length = 2 * strlen (document) + 1;
    char prepared_document[length];

    unsigned long prepared_length = mysql_real_escape_string(conn, prepared_document, document, strlen(document));

    MYSQL_BIND bind_structs[1];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].length = &prepared_length;
    bind_structs[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind_structs[0].is_null = 0;
    bind_structs[0].buffer = (void *) &prepared_document;
    bind_structs[0].buffer_length = strlen(prepared_document) * sizeof(char);

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

int main(int argc, char **argv) {
	try {
		const char *db_name;
		const char *db_user;
		const char *db_host;
		const char *db_passwd;
		char *filename;
		int c;
		if (argc < 9) {
				usage(argv);
				return 1;
		}
		while ((c = getopt (argc, argv, "d:u:h:p:")) != -1)
			switch (c)
		    {
				case 'd':
					db_name = optarg;
					break;
				case 'u':
					db_user = optarg;
					break;
				case 'h':
					db_host = optarg;
					break;
				case 'p':
					db_passwd = optarg;
					break;
				case '?':
		    	 usage(argv);
		         return 1;
		       default:
		         usage(argv);
		         return 1;
		    }

		if (optind == argc -1)
			filename = argv[optind];
		else {
			usage(argv);
			return 1;
		}

		MYSQL *conn = mysql_init(NULL);
		BOOST_SCOPE_EXIT( (conn) ) {
			mysql_close(conn);
		} BOOST_SCOPE_EXIT_END

		if (!mysql_real_connect(conn, db_host, db_user, db_passwd, db_name, 0, NULL, CLIENT_COMPRESS)) {
		        cerr << "Error: mysql_real_connect() failed to connect to `" << db_name << "`." << endl;
		        return EXIT_FAILURE;
		    }

		const GURL::Filename::UTF8 url(filename);
		GP<DjVuDocument> doc = DjVuDocument::create_wait(url);

		if (!doc) {
			std::cerr << "cannot open file" << std::endl;
			return -1;
		}

		int doc_id = store_document(filename, conn);
		return process_document(doc, doc_id, conn);
	}
	catch (exception& ex) {
		std::cout << "An exception occurred: " << ex.what() << "\n";
		return EXIT_FAILURE;
	}
	//return EXIT_SUCCESS;
}

