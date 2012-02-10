/*
 * export shapes
 *
 *  Created on: Jan 16, 2012
 *      Author: zasvid
 */

#include <iostream>
#include <exception>
#include <string>
#include <map>

#include <mysql.h>

#include <boost/scope_exit.hpp>

#include <DjVuDocument.h>
#include <DjVuImage.h>
#include <JB2Image.h>
#include <Arrays.h>
#include <GBitmap.h>
#include <BSByteStream.h>

using namespace std;

typedef std::pair<int, int> IntPair;

typedef struct {
	int right, top, left, bottom;
} BoundingBox;

BoundingBox
 compute_bounding_box(const GBitmap &bm)
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

int store_document (std::string document, MYSQL *pconn) {
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(pconn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO documents(document) VALUES (?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[1];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].buffer_type = MYSQL_TYPE_VAR_STRING;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &document;
    //bind_structs[0].length =
    //TODO: string doesn't load into database

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    return mysql_insert_id(pconn);
}

int store_page(int document_id, int page_number, MYSQL *pconn) {
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(pconn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO pages(document_id, page_number) VALUES (?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[2];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[0].is_unsigned = 1;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &document_id;

    bind_structs[1].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[0].is_unsigned = 1;
    bind_structs[1].is_null_value = 0;
    bind_structs[1].buffer = (void *) &page_number;

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    return mysql_insert_id(pconn);
}


/***
 *
 * store_blit stores a blit in the database
 * and returns it's autogenerated database index.
 *
 *
 */
int store_blit(int shape_id, int page_id, unsigned short b_left, unsigned short b_bottom, MYSQL *pconn) {
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(pconn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO blits(shape_id, b_left, b_bottom, page_id) VALUES (?,?,?,?)";
    if (mysql_stmt_prepare(pinsert_stmt, sql, strlen(sql)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql << "`:" << endl << mysql_stmt_error(pinsert_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[4];
    memset(bind_structs, 0, sizeof(bind_structs));

    bind_structs[0].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[0].is_unsigned = 1;
    bind_structs[0].is_null_value = 0;
    bind_structs[0].buffer = (void *) &shape_id;

    bind_structs[3].buffer_type = MYSQL_TYPE_LONG;
    //bind_structs[3].is_unsigned = 1;
    bind_structs[3].is_null_value = 0;
    bind_structs[3].buffer = (void *) &page_id;

    bind_structs[1].buffer_type = MYSQL_TYPE_SHORT;
    bind_structs[1].is_unsigned = 1;
    bind_structs[1].is_null_value = 0;
    bind_structs[1].buffer = (void *) &b_left;

    bind_structs[2].buffer_type = MYSQL_TYPE_SHORT;
    bind_structs[2].is_unsigned = 1;
    bind_structs[2].is_null_value = 0;
    bind_structs[2].buffer = (void *) &b_bottom;

    if (mysql_stmt_bind_param(pinsert_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    if (mysql_stmt_execute(pinsert_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed: " << endl << sql << endl;
        return EXIT_FAILURE;
    }

    return mysql_insert_id(pconn);
}

/***
 *
 * store_shape stores a shape in the database
 * and returns it's autogenerated database index.
 *
 */
int store_shape(GP<GBitmap> bits, int page_id, int parent, MYSQL *pconn) {
    GP<ByteStream> bs = ByteStream::create();
    BoundingBox bbox;
    if (bits) {
        bits->save_pbm(*bs);
        bbox = compute_bounding_box(*(bits));
        cout << bbox.top << ' ' << bbox.left << ' ' << bbox.right << ' ' << bbox.bottom << endl;
    }
    MYSQL_STMT *pinsert_stmt = mysql_stmt_init(pconn);
    BOOST_SCOPE_EXIT( (pinsert_stmt) ) {
        mysql_stmt_close(pinsert_stmt);
    } BOOST_SCOPE_EXIT_END

    const char sql[] = "INSERT INTO shapes(parent_id, bits, page_id, bbox_top, bbox_left, bbox_right, bbox_bottom) VALUES (?,?,?,?,?,?,?)";
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
    bind_structs[2].buffer = (void *) &page_id;

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
//    cout << "Inserted record #" << mysql_insert_id(pconn) << endl;
    return mysql_insert_id(pconn);
}

void export_shapes(GP<JB2Image> jimg, int page_id, MYSQL *pconn) {

    int sh_count = jimg->get_shape_count();
    int blit_count = jimg->get_blit_count();
    std::map<int, int> shape_id_translation; // translates from Dictionary indices to database entries
    //vector<ShapeNode *> shapes_nodes(sh_count);
    //QMultiHash<int, int> shapes_children;
    for (int i = 0; i < sh_count; i++) {
        JB2Shape shape = jimg->get_shape(i);

        // if shape.parent in shape.indices
        int parent_id = shape.parent >= 0 ? shape_id_translation[shape.parent] : shape.parent;

        int shape_id = store_shape(shape.bits, page_id, parent_id,pconn);
        shape_id_translation.insert(IntPair(i,shape_id));
    }
    for (int i = 0; i < blit_count; i++) {
    	JB2Blit *blit = jimg->get_blit(i);
    	//TODO: blity tylko istniejących kształtów?
    	if (blit && shape_id_translation[blit->shapeno]) {
    	    		store_blit(shape_id_translation[blit->shapeno], page_id, blit->left, blit->bottom, pconn);
    	}
    }

}

void usage(char **argv) {
	std::cout << "Usage: " << argv[0] << " -h <host> -d <database> -u <user> -p <password> <filename>" << std::endl;
}

int main(int argc, char **argv) {
	try {
		//const char *db_name = "exported_shapes";
		//const char *db_user = "tester";
		//const char *db_host = "localhost";
		//const char *db_passwd = "test";

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
		  /*       if (optopt == 'd' || optopt == 'p' || optopt == 'h' || optopt == 'u')
		        	 cerr << "Option -" << optopt << " requires an argument." << endl;
		         else if (isprint (optopt))
		        	 cerr << "Unknown option `-" << optopt <<"'." << endl;
		         else
		        	 cerr << "Unknown option character `" << optopt << "'." << endl;*/
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

		MYSQL *pconn = mysql_init(NULL);
		BOOST_SCOPE_EXIT( (pconn) ) {
			mysql_close(pconn);
		} BOOST_SCOPE_EXIT_END


		if (!mysql_real_connect(pconn, db_host, db_user, db_passwd, db_name, 0, NULL, CLIENT_COMPRESS)) {
		        cerr << "Error: mysql_real_connect() failed to connect to `" << db_name << "`." << endl;
		        return EXIT_FAILURE;
		    }

		const GURL::Filename::UTF8 url(filename);
		GP<DjVuDocument> doc = DjVuDocument::create_wait(url);
		if (!doc) {
			std::cerr << "cannot open file" << std::endl;
			return -1;
		}
		int doc_id = store_document(argv[1], pconn);

		const int pages = doc->get_pages_num();
		for(int page = 0; page < pages; page++) {
			std::cout << "processing page " << page << "... ";

			GP<DjVuImage> dimg = doc->get_page(page);

			int page_id = store_page(doc_id, page, pconn);

			if (!dimg) {
				std::cout << "get_page failed";
			} else {
				GP<JB2Image> jimg = dimg->get_fgjb();
				if (!jimg) {
					std::cout << "get_fgjb failed";
				} else {
					export_shapes(jimg, page_id, pconn);
				}
			}

			std::cout << std::endl;
		}

	}
	catch (exception& ex) {
		std::cout << "An exception occurred: " << ex.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
