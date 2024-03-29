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



    time_t tm, tm2;

    QUEUE queues[10];
    char rs_nm[256];
    char path[256];
    RESULT_DETAIL_T result_detail;
    char *x = "aklsjfklsjdflasjdlksjdflsajflsjflksjlfksjdflksjflksjdsldfkjsalkfjaslkdjflasdkjfalskfjslakfjlsjflksjdflkasjdflsjdflkjsdfljsflksjlfkjasdflkajsdflsjlfkjsdklfjsdlfkjslfkjslfdjsldkfjslkfjsldkjfsldkfjsldfj";

    for (int i = 0; i < 10; i++) {
        sprintf(rs_nm, "rs_%d", i);
        sprintf(path, "/Users/dk.choi/qtest/store/rs_%d.m", i);
        printf("-------\n");
        RESOURCE resource = mom_create_resource_file(rs_nm, path, MAX_CAPACITY);
        queues[i] = mom_create_shared_queue(resource, 1000000, 128, FALSE, &result_detail);
        //int x = mom_clear_shared_queue(queues[i], &result_detail);
        //printf("%d\n", x);
        //mom_destroy_resource(resource);
    }

    //exit(1);

    //RESOURCE_T* p_res = create_resource_shm("b",1000000L);

//    RESOURCE resource_q2 = mom_create_resource_shm("Q2.q",  MAX_CAPACITY);
//    mom_destroy_resource(resource_q2);
    //exit(1);

    //RESOURCE resource = mom_create_resource_file("c", "./list_", 1000000L);
//    if (resource_q1 == NULL) {
//        printf("resource is null");
//        return FAIL;
//    }
//    if (resource_q2 == NULL) {
//        printf("resource is null");
//        return FAIL;
//    }

    /*RESOURCE_T* p_res = create_resource_local(1000000L);*/


    fflush(stdout);
    char t[10];
    printf("111\n");
    fflush(stdout);

    char key[100];
    time(&tm);
    for (int i = 0; i < 100000; i++) {
        sprintf(key, "%d", i);
        if (mom_push_shared_queue(queues[mom_get_hash_idx(key,10)], key, strlen(key), &result_detail) < 0) {
            printf("error1 %s\n", result_detail.message);
        }
    }
    time(&tm2);
//    for (int i = 0; i < 10; i++) {
//        int m = mom_size_shared_queue(queues[i], &result_detail);
//        printf("ADD QUEUE %d %d %d\n", i, (tm2 - tm), m);
//    }


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
    int cnt = 0;
    for (int i = 0; i < 10; i++) {
        for (;;) {
            DATA data = mom_poll_shared_queue(queues[i], 1, &result_detail);
            if (data != NULL) {
                //printf("[%d]=====>>[%s]\n", i, data->data);
                mom_destroy_shared_data(data, &result_detail);
                //sleep(1);
                cnt++;
            } else {
                printf("poll end [%d]\n", i);
                break;
            }
        }
    }
    time(&tm2);
    //int m = mom_size_shared_queue(queues[0], &result_detail);
    printf("POLL QUEUE (%d)(%d) \n", (tm2 - tm), cnt);
//    while ((data = mom_poll_shared_queue(queue, 10000, &result_detail)) != NULL) {
//        printf("poll data %d %s\n", result_detail.fail, (STRING) data->data);
//        mom_destroy_shared_data(data,&result_detail);
//    }



//sleep(10);



    memset(&result_detail, 0x00, sizeof(RESULT_DETAIL_T));

    RESOURCE resource2 = mom_create_resource_local("dpp", MAX_CAPACITY);

//    mom_destroy_resource(resource2);
//    exit(1);

    MAP map = mom_create_shared_map(resource2, 2000000, 128, TRUE, &result_detail);
    if (map == NULL) {
        printf("error2 %s\n", result_detail.message);
    }

//    for(int i=0;i<BUCKET_SIZE;i++) {
//        printf("%d %d \n", i, map->header->bucket[i].cnt);
//    }



    printf("error2 %s\n", result_detail.message);

    time(&tm);

    //for(int j=0;j<1000;j++) {
    for (int i = 0; i < 10; i++) {
        char key[32];
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

    int rcnt = 0;
    for (int i = 0; i < 10; i++) {
        char key[32];
        sprintf(key, "%d", i);
        MAP_DATA map_data = mom_get_shared_map(map, key, &result_detail);

        if (map_data != NULL) {
            printf("%s %s %zu\n", key, (STRING) map_data->data, map_data->size);
            rcnt++;
        }

//        printf("%s %zu\n", result_detail.message, result_detail.code);

//        MAP_DATA map_data = mom_get_shared_map(map, "test", &result_detail);
//        if (map_data != NULL) {
//            printf("%s %zu", (STRING) map_data->data, map_data->size);
//        }
    }


    //}

    time(&tm2);
    printf("%d \n", (tm2 - tm));

    printf("===OK %d\n", rcnt);

    mom_remove_shared_map(map,"0", &result_detail);

    for(int k=0;k<10;k++) {

        printf("%d\n", mom_size_shared_map(map, &result_detail));

        MAP_KEYS rtn = mom_get_shared_map_keys(map, &result_detail);
        printf("%d\n", rtn->cnt);
        for (int i = 0; i < rtn->cnt; i++) {
            //printf("%s\n", rtn->keys[i]);
        }

        mom_free_shared_map_keys(rtn);

        sleep(1);

        int ss = mom_expire_shared_map(map,4,&result_detail);
        printf("expr %d\n", ss);
    }

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
