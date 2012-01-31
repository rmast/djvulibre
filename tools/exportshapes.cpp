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
#include <BSByteStream.h>
#include <GBitmap.h>
#include <GPixmap.h>


using namespace std;
/***
 *
 * Store shape returns the shape's index in the database.
 *
 */
int store_shape(GP<GBitmap> bits, int parent, MYSQL *pconn) {
    GP<ByteStream> bs = ByteStream::create();
    if (bits) {
        bits->save_pbm(*bs);
    }
    MYSQL_STMT *pinsert_into_shapes_stmt = mysql_stmt_init(pconn);
    BOOST_SCOPE_EXIT( (pinsert_into_shapes_stmt) ) {
        mysql_stmt_close(pinsert_into_shapes_stmt);
    } BOOST_SCOPE_EXIT_END

    //const char sql1[] = "INSERT INTO shapes(parent, data) VALUES (?,?)";
    const char sql1[] = "INSERT INTO shapes(bits) VALUES (?)";
    if (mysql_stmt_prepare(pinsert_into_shapes_stmt, sql1, strlen(sql1)) != 0) {
            cerr << "Error: mysql_stmt_prepare() failed to prepare `" << sql1 << "`:" << endl << mysql_stmt_error(pinsert_into_shapes_stmt) << endl;
            return EXIT_FAILURE;
    }

    MYSQL_BIND bind_structs[1];
    memset(bind_structs, 0, sizeof(bind_structs));

    unsigned long length0;
    bind_structs[0].length = &length0;
    bind_structs[0].buffer_type = MYSQL_TYPE_BLOB;
    bind_structs[0].is_null_value = 0;

    if (mysql_stmt_bind_param(pinsert_into_shapes_stmt, bind_structs) != 0) {
        cerr << "Error: mysql_stmt_bind_param() failed." << endl;
        return EXIT_FAILURE;
    }

    size_t bytes_to_read = bs->size();
    cout << "Bytes to read: " << bytes_to_read << endl;
    char buf[1024];
    bool finished = false;
    while (finished) {
           size_t res = bs->readall(buf,1024);
           if (res == 0) {
        	   finished = true;
           } else {
        	   cout << "Bytes read: " << res << endl;
        	   if (mysql_stmt_send_long_data(pinsert_into_shapes_stmt, 0, buf, res) != 0) {
        		   cerr << "Error: mysql_stmt_send_long_data() failed." << endl;
        		   return EXIT_FAILURE;
        	   }
           }
       }

    if (mysql_stmt_execute(pinsert_into_shapes_stmt) != 0) {
        cerr << "Error: mysql_stmt_execute() failed." << endl;
        return EXIT_FAILURE;
    }

    cout << "Inserted record #" << mysql_insert_id(pconn) << endl;

    return mysql_insert_id(pconn);

}

void export_shapes(GP<JB2Image> jimg, MYSQL *pconn) {

    int sh_count = jimg->get_shape_count();
  //  int blit_count = jimg->get_blit_count();

    //vector<ShapeNode *> shapes_nodes(sh_count);
    //QMultiHash<int, int> shapes_children;
    for (int i = 0; i < sh_count; i++) {
        JB2Shape shape = jimg->get_shape(i);
        int parent_id = shape.parent;
        //int shape_id =
        store_shape(shape.bits, parent_id,pconn);

        //int idx = shape.parent >= 0 ? shape.parent : -1;
      //  shapes_children.insert(idx, i);

        // the array holds the shape bitmap in the PBM format

     //   ShapeNode * node = new ShapeNode(i);
     //   node->getPixmap().loadFromData(reinterpret_cast<const uchar*>((char*)array), array.size());
    //    node->getPixmap().setMask(node->getPixmap().createMaskFromColor(Qt::white, Qt::MaskInColor)); //add transparency
        //boundingShapeSize = boundingShapeSize.expandedTo(node->getPixmap().size());

   //     shapes_nodes[i] = node;
    }
/**
    // now put blits
    for (int i = 0; i < blit_count; i++) {
        JB2Blit *blit = jimg->get_blit(i);
        if (blit && shapes_nodes[blit->shapeno]) {
            shapes_nodes[blit->shapeno]->addBlit(blit->left, blit->bottom);
            //qDebug("blit %d %u %u %u", i, blit->left, blit->bottom, blit->shapeno);
        }
    }

    QMultiHash<int, int>::iterator itr;

    //set parentness
    for (int parent_id = 0; parent_id < sh_count; parent_id++) {
        itr = shapes_children.find(parent_id);
        while (itr != shapes_children.end() && itr.key() == parent_id) {
            if (shapes_nodes[itr.value()])
                shapes_nodes[itr.value()]->setParent(shapes_nodes[parent_id]);
            ++itr;
        }
    }

    //find root shapes
    itr = shapes_children.find(-1);
    while (itr != shapes_children.end() && itr.key() == -1) {
        if (shapes_nodes[itr.value()]) {
            roots.second.append(shapes_nodes[itr.value()]);
            shapes_nodes[itr.value()]->caluculateTreeHeights();

            //DEBUG
            //if (shapes_nodes[itr.value()]->getToRootHeight() || shapes_nodes[itr.value()]->getToLeafHeight()) {
            //    *itr;
            //}
        }
        ++itr;
    };
    roots.first = sh_count;
**/
  //  qSort(roots.second.begin(), roots.second.end(), ShapeNode::greaterThan);

}

int main(int argc, char **argv) {
	try {
		MYSQL *pconn = mysql_init(NULL);
		BOOST_SCOPE_EXIT( (pconn) ) {
			mysql_close(pconn);
		} BOOST_SCOPE_EXIT_END

		const char *db_name = "exported_shapes";
		const char *db_user = "tester";
		const char *db_host = "localhost";
		const char *db_passwd = "test";

		if (!mysql_real_connect(pconn, db_host, db_user, db_passwd, db_name, 0, NULL, CLIENT_COMPRESS)) {
		        cerr << "Error: mysql_real_connect() failed to connect to `" << db_name << "`." << endl;
		        return EXIT_FAILURE;
		    }

		const GURL::Filename::UTF8 url(argv[1]);
		std::cout << "post-db\n";
		GP<DjVuDocument> doc = DjVuDocument::create_wait(url);
		std::cout << "post-crw\n";
		if (!doc) {
			std::cerr << "cannot open file" << std::endl;
			return -1;
		}
		const int pages = doc->get_pages_num();
		for(int page = 0; page < pages; page++) {
			std::cout << "processing page " << page << "... ";

			GP<DjVuImage> dimg = doc->get_page(page);
			if (!dimg) {
				std::cout << "get_page failed";
			} else {
				GP<JB2Image> jimg = dimg->get_fgjb();
				if (!jimg) {
					std::cout << "get_fgjb failed";
				} else {
					export_shapes(jimg, pconn);
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
