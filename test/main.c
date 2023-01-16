/*
 *  * main.c
 *
 *   *
 *    *  Created on: 2020. 5. 11.
 *     *      Author: 76050
 *      */

#include <unistd.h>
#include "mom_resource.h"
#include "mom_shared_queue.h"
#include "mom_result.h"
#include "mom_shared_map.h"
#include "mom_call_app.h"


int main() {

    time_t tm,tm2;


    //RESOURCE_T* p_res = create_resource_shm("b",1000000L);
    RESOURCE resource = mom_create_resource_shm("c", MAX_CAPACITY);

//    mom_destroy_resource(resource);
//    exit(1);

    //RESOURCE resource = mom_create_resource_file("c", "./list_", 1000000L);
    if (resource == NULL) {
        printf("resource is null");
        return FAIL;
    }

    /*RESOURCE_T* p_res = create_resource_local(1000000L);*/
    RESULT_DETAIL_T result_detail;


    QUEUE queue = mom_create_shared_queue(resource, 1000000, FALSE, &result_detail);

    fflush(stdout);
    char t[10];
    printf("111\n");
    fflush(stdout);

    char key[100];
    time(&tm);
    for (int i = 0; i < 100000; i++) {
        sprintf(key, "%d", i);
        if (mom_add_shared_queue(queue, key, strlen(key), &result_detail) < 0) {
            printf("error1 %s\n", result_detail.message);
        }
    }
    time(&tm2);
    int m = mom_size_shared_queue(queue, &result_detail);
    printf("ADD QUEUE %d %d\n",  (tm2 - tm), m);
//    int m = mom_size_shared_queue(queue, &result_detail);
//    {
//        DATA data = mom_get_shared_queue(queue, 0, &result_detail);
//        if (data != NULL) {
//            printf("get data %s %d\n", (STRING) data->data, m);
//        }
//    }
//
//    for (int i = 0; i < 10000; i++) {
//        DATA data = mom_get_shared_queue(queue, i, &result_detail);
//        if (data != NULL) {
//            printf("get data %s %d\n", (STRING) data->data, m);
//        }
//    }

    //DATA data = NULL;
    time(&tm);
    for(;;) {
        DATA data = mom_poll_shared_queue(queue, 1000000, &result_detail);
        if(data != NULL) {
            printf("DATA %s\n",data->data);
            mom_destroy_shared_data(data,&result_detail);
            //sleep(1);
        } else {
            printf("poll end [%s][%s][%d]\n", result_detail.message, result_detail.fn, result_detail.line);
            break;
        }
    }
    time(&tm2);
    m = mom_size_shared_queue(queue, &result_detail);
    printf("POLL QUEUE %d(%d) \n",  (tm2 - tm),m);
//    while ((data = mom_poll_shared_queue(queue, 10000, &result_detail)) != NULL) {
//        printf("poll data %d %s\n", result_detail.fail, (STRING) data->data);
//        mom_destroy_shared_data(data,&result_detail);
//    }


sleep(10);



    memset(&result_detail, 0x00, sizeof(RESULT_DETAIL_T));

    RESOURCE resource2 = mom_create_resource_shm("d", MAX_CAPACITY);

    mom_destroy_resource(resource2);
    exit(1);

    MAP map = mom_create_shared_map(resource2, 2000000, TRUE, &result_detail);
    if (map == NULL) {
        printf("error2 %s\n", result_detail.message);
    }

//    for(int i=0;i<BUCKET_SIZE;i++) {
//        printf("%d %d \n", i, map->header->bucket[i].cnt);
//    }


    char *x = "aklsjfklsjdflasjdlksjdflsajflsjflksjlfksjdflksjflksjdsldfkjsalkfjaslkdjflasdkjfalskfjslakfjlsjflksjdflkasjdflsjdflkjsdfljsflksjlfkjasdflkajsdflsjlfkjsdklfjsdlfkjslfkjslfdjsldkfjslkfjsldkjfsldkfjsldfj";

    printf("error2 %s\n", result_detail.message);

    time(&tm);

    //for(int j=0;j<1000;j++) {
    for (int i = 0; i < 200000; i++) {

        sprintf(key, "%d", i);
        mom_put_shared_map(map, key, x, strlen(x), &result_detail);

        //if(result_detail.code != SUCCESS) {
        //    printf("%s %s\n", key, result_detail.message);
        //}

//        MAP_DATA map_data = mom_get_shared_map(map, "test", &result_detail);
//        if (map_data != NULL) {
//            printf("%s %zu", (STRING) map_data->data, map_data->size);
//        }
    }
    //}


    time(&tm2);

    printf("%d \n", (tm2 - tm));


    time(&tm);


    //for(int j=0;j<100;j++) {

    for (int i = 0; i < 200000; i++) {

        sprintf(key, "%d", i);
        MAP_DATA map_data = mom_get_shared_map(map, key, &result_detail);

//            if (map_data != NULL) {
//                printf("%s %zu\n", (STRING) map_data->data, map_data->size);
//            }

//        printf("%s %zu\n", result_detail.message, result_detail.code);

//        MAP_DATA map_data = mom_get_shared_map(map, "test", &result_detail);
//        if (map_data != NULL) {
//            printf("%s %zu", (STRING) map_data->data, map_data->size);
//        }
    }


    //}

    time(&tm2);
    printf("%d %d \n", (60000 * 100), (tm2 - tm));

    printf("===OK\n");

    printf("%d\n", mom_size_shared_map(map, &result_detail));

    mom_destroy_shared_map(map, &result_detail);

    //mom_destroy_resource(resource2);


//	for(int i=0;i<m;i++){
//
//		int rs;
//		DATA_T* p_data = mom_poll_shared_queue(q,0,&rs);
//		if(p_data != NULL)
//			printf("poll data %d %s\n", rs, p_data->p_data);
//			//destroy_shared_data(p_data);
//	}

/*
    RESOURCE_T* p_res2 = create_resource_file("b","map.x",1000000L);

	MAP_T* map = create_map(p_res2,512);

	map->put(map,"tee","1234",4,NULL);

	printf("PUT ==%d\n", map->count(map,NULL));
	fflush(stdout);

	MAP_DATA_T* map_data = map->get(map,"tee",NULL);



	printf("GET ==\n");

	printf("%s \n", map_data->p_data);

		destroy_shared_data((DATA_T*)map_data);


		callapp("../app","libapp.so","flow");

		//map->remove(map,"tee",NULL);

	//printf("%d %d %d %d\n",sizeof(DATA_T),sizeof(size_t),sizeof(char*),sizeof(MAP_DATA_T));

*/

    /*
    printf("112\n");
    fflush(stdout);

    if(q->add(q,"bbb2",4) != 0){
        printf("error\n");
    }
    printf("113\n");
    fflush(stdout);
    if(q->add(q,"bbb3",4) != 0){
        printf("error\n");
    }

    destroy_queue(q);

  RESOURCE_T* p_res2 = create_resource_file("a","map.x",1000000L);
  QUEUE_T* q2 = create_queue(p_res2,5);

    printf("114\n");
    fflush(stdout);
    q2->remove(q2,t,4,2);

    printf("115\n");
    fflush(stdout);

    if(q2->add(q2,"bbb4",4) != 0){
        printf("error\n");
    }
    printf("116\n");
    fflush(stdout);
    if(q2->add(q2,"bbb5",4) != 0){
        printf("error\n");
    }

    if(q2->add(q2,"bbb6",4) != 0){
        printf("error\n");
    }

    q2->clear(q2);

    if(q2->add(q2,"bbb4",4) != 0){
        printf("error\n");
    }

    if(q2->add(q2,"bbb5",4) != 0){
        printf("error\n");
    }

    if(q2->add(q2,"bbb6",4) != 0){
        printf("error\n");
    }



    memset(t,0x00,sizeof(t));
    printf("get idx %s\n", q2->get(q2,t,sizeof(t),2));

    printf("get count %d\n\n", q2->count(q2));

fflush(stdout);



    int m = q2->count(q2);


    for(int i=0;i<m;i++){
        memset(t,0x00,sizeof(t));
        printf("data %s\n", q2->poll(q2,t,sizeof(t)));
        printf("get count %d\n", q2->count(q2));
    }

fflush(stdout);
*/

/*
 * 	destroy_queue(q);
 *
 * 		destroy_resource(p_res);
 *
 * 			*/


}
