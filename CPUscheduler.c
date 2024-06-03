#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define NO_PROCESS 99                   // constant to use when there is no available process to be executed

#define FCFS_SCHED 0                    // constant to represent the first come, first served algorithm
#define SJF_SCHED 1                     // constant to represent the shortest job first algorithm
#define PRI_SCHED 2                     // constant to represent the priority based algorithm
#define RR_SCHED 3                      // constant to represent the round robin based algorithm

typedef struct {
    int process_arrival_time;       // arrival time
    int process_priority;           // priority. lower numbers mean higher priority. assume to be unique.
    int how_much_left;              // CPU time left to complete the process
    int waiting_time;               // how much the process has waited without attention from the CPU
    int response_time;              // how much the process has waited until first attention from the CPU
    int turnaround_time;            // process finish time - process arrival time
    int has_run_at_least_once;      // used to track if the process has already run at least once. used for calculating response times.
    int has_terminated;             // used to track if a process has terminated.
    int process_number;             // used to store the process number
    int io_how_much_left;           // I/O burst time left to complete the process
    int io_burst_intime;
} pinfo;

pinfo **allocate_mem(int length) {
    pinfo **pinfos = malloc(sizeof(pinfo *) * length);
    for (int i = 0; i < length; ++i) {
        pinfos[i] = (pinfo*)malloc(sizeof(pinfo));
    }
    return pinfos;
}

void deallocate_mem(pinfo** pinfos, int length) {
    for (int i = 0; i < length; ++i) {
        free(pinfos[i]);
    }
    free(pinfos);
}

typedef struct node {
    int pinfo_num;
    struct node *next;
} node;

typedef struct {
    node *head;
    node *tail;
} queue; 

void enqueue(queue *q, int pinfo_num) {
    node *new_node = (node *)malloc(sizeof(node));
    new_node->pinfo_num = pinfo_num;
    new_node->next = NULL;
    if (q->tail) {
        q->tail->next = new_node;
    } else {
        q->head = new_node;
    }
    q->tail = new_node;
}

int dequeue(queue *q) {
    if (!q->head) return NO_PROCESS; // 큐가 비어있으면 NO_PROCESS 을 반환
    node *temp = q->head;
    int pinfo_num = temp->pinfo_num;
    q->head = q->head->next;
    if (!q->head) {
        q->tail = NULL;
    }
    free(temp);
    return pinfo_num; // 큐가 비어있지 않으면 pinfo_num을 반환
}

int is_queue_empty(queue *q) {
    return q->head == NULL;
}

void display_queue(queue *q) {
    node *current = q->head;
    while (current) {
        printf("P%d ", current->pinfo_num+1);
        current = current->next;
    }
    printf("\n");
}

void take_input(pinfo ***pinfos_out, int *pinfos_len) { //create process - 데이터 입력
    int nprocs;
    pinfo **pinfos = NULL;

    printf("Enter the number of processes: ");
    scanf("%d", &nprocs);
    *pinfos_len = nprocs;

    pinfos = allocate_mem(nprocs);

    for (int i = 0; i < nprocs; ++i) {
        printf("\n");
        pinfos[i]->process_priority = 0;
        pinfos[i]->waiting_time = 0;
        pinfos[i]->response_time = 0;
        pinfos[i]->turnaround_time = 0;
        pinfos[i]->has_run_at_least_once = 0;
        pinfos[i]->has_terminated = 0;
        pinfos[i]->process_number = i;
        pinfos[i]->io_burst_intime = 0;
        pinfos[i]->io_how_much_left = 0;

        printf("Enter priority of P%d: ", i+1);
        scanf("%d", &pinfos[i]->process_priority);

        printf("Enter time of arrival of P%d: ", i+1);
        scanf("%d", &pinfos[i]->process_arrival_time);

        printf("Enter burst time of P%d: ", i+1);
        scanf("%d", &pinfos[i]->how_much_left);
    }

    *pinfos_out = pinfos;
    printf("\n");
}

void display_output(pinfo **pinfos, int pinfos_len, int context_switches) {
    int avg_wt = 0;
    int avg_ta = 0;
    printf("\n");
    for (int i = 0; i < pinfos_len; ++i) {
        printf("\nP%d response time: %d\n", i+1, pinfos[i]->response_time);
        printf("P%d waiting time: %d\n", i+1, pinfos[i]->waiting_time);
        printf("P%d turnaround time: %d\n", i+1, pinfos[i]->turnaround_time);
        avg_wt += pinfos[i]->waiting_time;
        avg_ta += pinfos[i]->turnaround_time;
    }
    //printf("\nNumber of Context Switches: %d\n", context_switches);
    printf("\nAverage Waiting Time: %.2f\n", (double)avg_wt / pinfos_len);
    printf("\nAverage Turnaround Time: %.2f\n", (double)avg_ta / pinfos_len);

}

int has_process_finished(pinfo **pinfos, int process) { //프로세스가 끝났는지 확인
    if (process == NO_PROCESS) 
        return 1;
    return pinfos[process]->has_terminated;
}

int check_preemptive_sjf(pinfo **pinfos, int run_process, queue *q) {
    if (q->head) {
        if (pinfos[run_process]->how_much_left > pinfos[q->head->pinfo_num]->how_much_left) {
            //printf("run process: %d queue: %d\n",pinfos[run_process]->how_much_left, pinfos[q->head->pinfo_num]->how_much_left);
            return 1;
        }
    }
    else return 0;

}

int check_preemptive_prior(pinfo **pinfos, int run_process, queue *q) {
    if (q->head) {
        if (pinfos[run_process]->process_priority > pinfos[q->head->pinfo_num]->process_priority) {
            //printf("run process: %d queue: %d\n",pinfos[run_process]->how_much_left, pinfos[q->head->pinfo_num]->how_much_left);
            return 1;
        }
    }
    else return 0;

}

void order_sjf(pinfo **pinfos, queue *q) { //bubble sort
    if (is_queue_empty(q)) return;
    int swapped;
    node *ptr1;
    node *lptr = NULL;

    // Checking for empty list
    if (q->head == NULL)
        return;

    do {
        swapped = 0;
        ptr1 = q->head;

        while (ptr1->next != lptr) {
            if (pinfos[ptr1->pinfo_num]->how_much_left > pinfos[ptr1->next->pinfo_num]->how_much_left) {
                int temp = ptr1->pinfo_num;
                ptr1->pinfo_num = ptr1->next->pinfo_num;
                ptr1->next->pinfo_num = temp;
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);


}

void order_prior(pinfo **pinfos, queue *q) {
    if (is_queue_empty(q)) return;
    int swapped;
    node *ptr1;
    node *lptr = NULL;

    // Checking for empty list
    if (q->head == NULL)
        return;

    do {
        swapped = 0;
        ptr1 = q->head;

        while (ptr1->next != lptr) {
            if (pinfos[ptr1->pinfo_num]->process_priority > pinfos[ptr1->next->pinfo_num]->process_priority) {
                int temp = ptr1->pinfo_num;
                ptr1->pinfo_num = ptr1->next->pinfo_num;
                ptr1->next->pinfo_num = temp;
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);
}


void set_ready_queue(pinfo **pinfos, int pinfos_len, int time, queue *q, int method)
{
    for (int i = 0; i < pinfos_len; ++i) {
        if (pinfos[i]->process_arrival_time == time) {
                enqueue(q, pinfos[i]->process_number);
        }
    } //fcfs, rr

    if (method == SJF_SCHED) {
        order_sjf(pinfos, q); //sjf order 방식 적용
    }

    if (method == PRI_SCHED) {
        order_prior(pinfos, q); //rr order 방식 적용
    }
}


int all_process_finished(pinfo **pinfos, int pinfos_len) {
    for (int i = 0; i < pinfos_len; i++) {
        if (pinfos[i]->has_terminated == 0) 
            return 0;
    }
    return 1;
}

int has_process_arrived(pinfo **pinfos, int process, int time) {
    if (process == NO_PROCESS)
        return 1;
    return pinfos[process]->process_arrival_time <= time;
}

void run_process_ad_update_structs(int process_to_run, pinfo **pinfos, int pinfos_len, int *time, int *gantt, int *gant_len) {
    if (process_to_run != NO_PROCESS) {
        --pinfos[process_to_run]->how_much_left;
        pinfos[process_to_run]->has_run_at_least_once = 1;
        if (pinfos[process_to_run]->how_much_left <= 0) {
            pinfos[process_to_run]->has_terminated = 1;
            pinfos[process_to_run]->turnaround_time = *time - pinfos[process_to_run]->process_arrival_time + 1;
        }
    }
    gantt[*gant_len] = process_to_run; // 간트차트에 표시
    ++(*gant_len);

    for (int i = 0; i < pinfos_len; ++i) {
        if (i == process_to_run)
            continue;
        
        if (has_process_arrived(pinfos, i, *time) && (!pinfos[i]->has_run_at_least_once)) {
            ++pinfos[i]->response_time;
        }

        if (has_process_arrived(pinfos, i, *time) && (!pinfos[i]->has_terminated)) {
            ++pinfos[i]->waiting_time;
        }
    }
    ++*time;
}


void display_gantt(int *gantt, int gant_len) {
    
    int start = 0;

    printf("\nGantt Chart:\n");

    for (int i = 0; i < gant_len; ++i) {
        printf("===");
    }
    printf("==\n");

    // 간트 차트 출력
    while (start < gant_len) {

        if (gantt[start] == NO_PROCESS) {  // run 상태의 process가 없을 때
            printf("|||");
            start++;
            
        }
        else {
            int end = start;
            // 연속된 작업 구간 찾기
            while (end < gant_len && gantt[end] == gantt[start]) {
                end++;
            }

            // 작업 표시
            printf("|P%d", gantt[start]+1);
            // 해당 구간의 길이만큼 공백을 추가
            for (int i = start + 1; i < end; i++) {
                printf("   "); // 작업 번호와 동일한 간격 유지
            }
            start = end;
        }
    }
    printf(" |\n");

    for (int i = 0; i < gant_len; ++i) {
            printf("===");
    }
    printf("==\n");


    // 시간 표시
    start = 0;
    while (start < gant_len) {
        int end = start;
        // 연속된 작업 구간 찾기
        while (end < gant_len && gantt[end] == gantt[start]) {
            end++;
        }

        // 시작 시간 표시
        printf("%d  ", start);

        // 해당 구간의 길이만큼 공백을 추가
        for (int i = start + 1; i < end; i++) {
            printf("   "); // 각 시간 간격을 넓게 유지
        }

        start = end;
    }
    // 마지막 시간 표시
    printf("%d\n", gant_len);
    
    printf("\n\n");
    
}

int need_io(pinfo** pinfos, int run_process, int time) { //I/O 할 게 있는지 확인
    if (pinfos[run_process]->io_burst_intime == time) {
        return  1;
    } 
    else return 0;
}

void wait_process_ad_update_structs(int run_process, pinfo** pinfos, queue* ready, queue *wait) { // wait queue 상태 업데이트
    if (!is_queue_empty(wait)) {
        node *q = wait->head;
        int tmp = q->pinfo_num;
        --pinfos[q->pinfo_num]->io_how_much_left;
        if (pinfos[q->pinfo_num]->io_how_much_left == 0) {
            dequeue(wait);
            enqueue(ready, tmp);
        }
    }
}

void sjf(pinfo** pinfos, int pinfos_len, int preemptive) {
    queue ready = {NULL, NULL};
    queue wait = {NULL, NULL};
    int run_process = NO_PROCESS;
    int time = 0;
    int context_switches = 0;
    int do_preemptive = 0;
    int method = SJF_SCHED;
    int gantt[100];
    int gant_len = 0;
    int do_wait = 0; //wait을 해야하는지 여부 확인

    set_ready_queue(pinfos, pinfos_len, time, &ready, method);

    while(all_process_finished(pinfos, pinfos_len) == 0) {
        if (run_process == NO_PROCESS) { //현재 run할게 없을 때
            run_process = dequeue(&ready); 

            if (run_process == NO_PROCESS) { //현재 ready큐에 아무것도 없을 때
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            
        }

        if (has_process_finished(pinfos, run_process)) { //한 프로세스가 끝났음
            if (do_preemptive) { //process가 끝나서 다음 process 고르는 과정과 preemptive 겹치는 거 방지
                do_preemptive = 0;   
            }
            run_process = dequeue(&ready);

            if (run_process == NO_PROCESS) { //현재 ready큐에 아무것도 없을 때
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            else { //ready큐에서 하나 가져와서 run으로
                //printf("Context switch from one process to another\n");
                ++context_switches;
            }

        }

        if (do_preemptive==1) {
            enqueue(&ready, run_process);
            //display_queue(&ready);
            run_process = dequeue(&ready);
            //printf("Context switch from one process to another --by preemptive\n");
            ++context_switches;
            //do_preemptive = 0;

        }

        if (do_wait) {  //i/o wait 발생. 현재 프로세스는 wait queue에 넣고. ready큐에서 run할 프로세스 가져오기
            //printf("Context switch from one process to another\n");
            enqueue(&wait, run_process);
            run_process = dequeue(&ready);
            if (run_process == NO_PROCESS ) {
                ++time;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            do_wait = 0;
        }

        if (run_process != NO_PROCESS) { //정상적인 run
            while (pinfos[run_process]->how_much_left) {
                do_preemptive = 0;
                //printf("From time = %d to time = %d. Running P%d\n", time, time + 1, run_process+1);
                run_process_ad_update_structs(run_process, pinfos, pinfos_len, &time, gantt, &gant_len); //여기때문에 process 끝났는데 do_preemptive =1 일수 있음
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);

  
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

                if (preemptive) {
                    do_preemptive = check_preemptive_sjf(pinfos, run_process, &ready); // 
                    if (do_preemptive==1) {
                        break;
                    }
                }

                do_wait = need_io(pinfos, run_process, time);
                if (do_wait) break;
            }
        }
        
    }

    display_output(pinfos, pinfos_len, context_switches);

    display_gantt(gantt, gant_len);
}


void prior(pinfo** pinfos, int pinfos_len, int preemptive) {
    queue ready = {NULL, NULL};
    queue wait = {NULL, NULL};
    int run_process = NO_PROCESS;
    int time = 0;
    int context_switches = 0;
    int do_preemptive = 0;
    int method = PRI_SCHED;
    int gantt[100];
    int gant_len = 0;
    int do_wait = 0; //wait을 해야하는지 여부 확인

    set_ready_queue(pinfos, pinfos_len, time, &ready, method);

    while(all_process_finished(pinfos, pinfos_len) == 0) {
        if (run_process == NO_PROCESS) { //현재 run할게 없을 때
            run_process = dequeue(&ready); 

            if (run_process == NO_PROCESS) { //현재 ready큐에 아무것도 없을 때
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            
        }

        if (has_process_finished(pinfos, run_process)) { //한 프로세스가 끝났음
            if (do_preemptive) { //process가 끝나서 다음 process 고르는 과정과 preemptive 겹치는 거 방지
                do_preemptive = 0;   
            }
            run_process = dequeue(&ready);

            if (run_process == NO_PROCESS) { //현재 ready큐에 아무것도 없을 때
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            else { //ready큐에서 하나 가져와서 run으로
                //printf("Context switch from one process to another\n");
                ++context_switches;
            }

        }

        if (do_preemptive==1) {
            enqueue(&ready, run_process);
            //display_queue(&ready);
            run_process = dequeue(&ready);
            //printf("Context switch from one process to another --by preemptive\n");
            ++context_switches;
            //do_preemptive = 0;

        }

        if (do_wait) {  //i/o wait 발생. 현재 프로세스는 wait queue에 넣고. ready큐에서 run할 프로세스 가져오기
            //printf("Context switch from one process to another\n");
            enqueue(&wait, run_process);
            run_process = dequeue(&ready);
            if (run_process == NO_PROCESS ) {
                ++time;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            do_wait = 0;
        }

        if (run_process != NO_PROCESS) { //정상적인 run
            while (pinfos[run_process]->how_much_left) {
                do_preemptive = 0;
                //printf("From time = %d to time = %d. Running P%d\n", time, time + 1, run_process+1);
                run_process_ad_update_structs(run_process, pinfos, pinfos_len, &time, gantt, &gant_len); //여기때문에 process 끝났는데 do_preemptive =1 일수 있음
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);

                // printf("\nready:");
                // display_queue(&ready);
                // printf("\nwait:");
                // display_queue(&wait);
                // printf("\n");

                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

                if (preemptive) {
                    do_preemptive = check_preemptive_prior(pinfos, run_process, &ready); // 
                    if (do_preemptive==1) {
                        break;
                    }
                }

                do_wait = need_io(pinfos, run_process, time);
                if (do_wait) break;
            }
        }
        
    }

    display_output(pinfos, pinfos_len, context_switches);

    display_gantt(gantt, gant_len);
}



void fcfs(pinfo** pinfos, int pinfos_len,int io_burst) {
    queue ready = {NULL, NULL};
    queue wait = {NULL, NULL};
    int run_process = NO_PROCESS;
    int time = 0;
    int context_switches = 0;
    int method = FCFS_SCHED;
    int gantt[1000]; 
    int gant_len = 0;
    int do_wait = 0;

    set_ready_queue(pinfos, pinfos_len, time, &ready, method);
    while(all_process_finished(pinfos, pinfos_len) == 0) {
        if (run_process == NO_PROCESS) {
            
            run_process = dequeue(&ready); 
            
            if (run_process == NO_PROCESS) {
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS; // 
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            
        }

        

        if (has_process_finished(pinfos, run_process)) {
            run_process = dequeue(&ready);
            
            if (run_process == NO_PROCESS) {
                
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS; // 
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            else {
                //printf("Context switch from one process to another\n");
                ++context_switches;
            }
        }

        if (do_wait) {
            //printf("Context switch from one process to another\n");
            enqueue(&wait, run_process);
            run_process = dequeue(&ready);
            if (run_process == NO_PROCESS ) {
                ++time;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            do_wait = 0;
        }

        


        if (run_process != NO_PROCESS) { //run
            while (pinfos[run_process]->how_much_left) {
                //printf("From time = %d to time = %d. Running P%d\n", time, time + 1, run_process+1);
                run_process_ad_update_structs(run_process, pinfos, pinfos_len, &time, gantt, &gant_len);
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);

                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);



                do_wait = need_io(pinfos, run_process, time);
                if (do_wait) break;
            }
        }

        

    }

    display_output(pinfos, pinfos_len, context_switches);
    display_gantt(gantt, gant_len);

}


void copy_pinfo(pinfo* src, pinfo* dest) {
    memcpy(dest, src, sizeof(pinfo));
}


void rr(pinfo** pinfos, int pinfos_len, int quant) {
    queue ready = {NULL, NULL};
    queue wait = {NULL, NULL};
    int run_process = NO_PROCESS;
    int time = 0;
    int context_switches = 0;
    int method = RR_SCHED;
    int gantt[100]; 
    int gant_len = 0;
    int tq_expired = 0;
    int do_wait = 0;

    set_ready_queue(pinfos, pinfos_len, time, &ready, method);

    while(all_process_finished(pinfos, pinfos_len) == 0) {
        if (run_process == NO_PROCESS) {
            run_process = dequeue(&ready); //dequeue할게 없다면 all process finished?

            if (run_process==NO_PROCESS) {
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            
        }
        if (has_process_finished(pinfos, run_process)) {
            if (tq_expired) { //process가 끝나서 다음 process 고르는 과정과 tq 끝나서 고르는거 겹치는 거 방지
                tq_expired = 0;   
            }
            run_process = dequeue(&ready);
            if (run_process==NO_PROCESS) {
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            else {
            //printf("Context switch from one process to another\n");
            ++context_switches;
            }
        }

        if (tq_expired) {
            enqueue(&ready, run_process);
            run_process = dequeue(&ready);
            //printf("Context switch from one process to another -- by round robin\n"); 
            ++context_switches;
            tq_expired = 0;
        }

        if (do_wait) {  //i/o wait 발생. 현재 프로세스는 wait queue에 넣고. ready큐에서 run할 프로세스 가져오기
            //printf("Context switch from one process to another\n");
            enqueue(&wait, run_process);
            run_process = dequeue(&ready);
            if (run_process == NO_PROCESS ) {
                ++time;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

            }
            do_wait = 0;
        }

        if(run_process != NO_PROCESS) {
            int how_much_left_after_tq = pinfos[run_process]->how_much_left - quant;
            while (pinfos[run_process]->how_much_left) {

                //printf("From time = %d to time = %d. Running P%d\n", time, time + 1, run_process+1);
                run_process_ad_update_structs(run_process, pinfos, pinfos_len, &time, gantt, &gant_len);
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);

                wait_process_ad_update_structs(run_process, pinfos, &ready, &wait);

                if (pinfos[run_process]->how_much_left == how_much_left_after_tq) { //  time quantum 만큼 다 돌았으면 다음 process로 변경
                    tq_expired = 1;
                    break;
                }
                do_wait = need_io(pinfos, run_process, time);
                if (do_wait) break;
            }
        }

    }

    display_output(pinfos, pinfos_len, context_switches);

    display_gantt(gantt, gant_len);

}


void rr_prior(pinfo** pinfos, int pinfos_len, int quant) {
    queue ready = {NULL, NULL};
    int run_process = NO_PROCESS;
    int time = 0;
    int context_switches = 0;
    int method = PRI_SCHED;
    int gantt[100]; 
    int gant_len = 0;
    int tq_expired = 0;

    set_ready_queue(pinfos, pinfos_len, time, &ready, method);

    while(all_process_finished(pinfos, pinfos_len) == 0) {
        if (run_process == NO_PROCESS) {
            run_process = dequeue(&ready); //dequeue할게 없다면 all process finished?
            

            if (run_process==NO_PROCESS) {
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
            }
            
        }
        if (has_process_finished(pinfos, run_process)) {
            run_process = dequeue(&ready);
            if (run_process==NO_PROCESS) {
                time++;
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);
                gantt[gant_len] = NO_PROCESS;
                ++(gant_len);
            }
            else {
            //printf("Context switch from one process to another\n");
            ++context_switches;
            }
        }

        if (tq_expired) {
            enqueue(&ready, run_process);
            set_ready_queue(pinfos, pinfos_len, time, &ready, method);
            run_process = dequeue(&ready);
            //printf("Context switch from one process to another -- by round robin\n"); 
            ++context_switches;
            tq_expired = 0;
        }

        if(run_process != NO_PROCESS) {
            int how_much_left_after_tq = pinfos[run_process]->how_much_left - quant;
            while (pinfos[run_process]->how_much_left) {

                //printf("From time = %d to time = %d. Running P%d\n", time, time + 1, run_process+1);
                run_process_ad_update_structs(run_process, pinfos, pinfos_len, &time, gantt, &gant_len);
                set_ready_queue(pinfos, pinfos_len, time, &ready, method);

                if (pinfos[run_process]->how_much_left == how_much_left_after_tq) { //  time quantum 만큼 다 돌았으면 다음 process로 변경
                    tq_expired = 1;
                    break;
                }
                
            }
        }

    }

    display_output(pinfos, pinfos_len, context_switches);

    display_gantt(gantt, gant_len);

}

double hrr_ratio(int waiting_t, int burst_t) {
    return (double) (waiting_t + burst_t) / burst_t;  // (W+S) / S
}

void order_hrr(pinfo **pinfos, queue *q) {
    if (is_queue_empty(q)) return;
    int swapped;
    node *ptr1;
    node *lptr = NULL;

    // Checking for empty list
    if (q->head == NULL)
        return;

    do {
        swapped = 0;
        ptr1 = q->head;

        while (ptr1->next != lptr) {
            if (hrr_ratio(pinfos[ptr1->pinfo_num]->waiting_time, pinfos[ptr1->pinfo_num]->how_much_left) < hrr_ratio(pinfos[ptr1->next->pinfo_num]->waiting_time, pinfos[ptr1->next->pinfo_num]->how_much_left)) {
                int temp = ptr1->pinfo_num;
                ptr1->pinfo_num = ptr1->next->pinfo_num;
                ptr1->next->pinfo_num = temp;
                swapped = 1;
            }
            ptr1 = ptr1->next;
        }
        lptr = ptr1;
    }
    while (swapped);


}

void set_ready_queue_hrr(pinfo **pinfos, int pinfos_len, int time, queue *q)
{
    for (int i = 0; i < pinfos_len; ++i) {
        if (pinfos[i]->process_arrival_time == time) {
                enqueue(q, pinfos[i]->process_number);
        }
    } //fcfs, rr

    order_hrr(pinfos, q);
}

void hrr(pinfo** pinfos, int pinfos_len) {
    queue ready = {NULL, NULL};
    queue wait = {NULL, NULL};
    int run_process = NO_PROCESS;
    int time = 0;
    int context_switches = 0;
    int method = FCFS_SCHED;
    int gantt[1000]; 
    int gant_len = 0;

    set_ready_queue_hrr(pinfos, pinfos_len, time, &ready);
    while(all_process_finished(pinfos, pinfos_len) == 0) {
        if (run_process == NO_PROCESS) {
            
            run_process = dequeue(&ready); 
            
            if (run_process == NO_PROCESS) {
                time++;
                set_ready_queue_hrr(pinfos, pinfos_len, time, &ready);
                gantt[gant_len] = NO_PROCESS; // 
                ++(gant_len);
                

            }
            
        }

        

        if (has_process_finished(pinfos, run_process)) {
            run_process = dequeue(&ready);
            
            if (run_process == NO_PROCESS) {
                
                time++;
                set_ready_queue_hrr(pinfos, pinfos_len, time, &ready);
                gantt[gant_len] = NO_PROCESS; // 
                ++(gant_len);
                

            }
            else {
                //printf("Context switch from one process to another\n");

            }
        }

        

        


        if (run_process != NO_PROCESS) { //run
            while (pinfos[run_process]->how_much_left) {
                //printf("From time = %d to time = %d. Running P%d\n", time, time + 1, run_process+1);
                run_process_ad_update_structs(run_process, pinfos, pinfos_len, &time, gantt, &gant_len);
                set_ready_queue_hrr(pinfos, pinfos_len, time, &ready);





                
            }
        }

        

    }

    display_output(pinfos, pinfos_len, context_switches);
    display_gantt(gantt, gant_len);

}



int main() {
    pinfo **pinfos = NULL;
    int algo_num = 8;
    int pinfos_len = 0;
    int preemptive = 1;
    int quant = 0;
    int io_burst = 0;

    take_input(&pinfos, &pinfos_len);

    pinfo **pinfos_copies[8];
    for (int i = 0; i < algo_num; ++i) {
        pinfos_copies[i] = allocate_mem(pinfos_len);
        for (int j = 0; j < pinfos_len; ++j) {
            copy_pinfo(pinfos[j], pinfos_copies[i][j]);
        }
    }

    printf("Enter whether I/O Burst or not (0 / 1): \n");
    scanf("%d", &io_burst);
    if (io_burst) {
        for (int i = 0; i < algo_num-2; ++i) {
            pinfos_copies[i][0]->io_burst_intime = 1; // 첫번째 process에 대해서만, cpu burst하고 1초 뒤 I/O burst
            pinfos_copies[i][0]->io_how_much_left = pinfos_copies[i][0]->how_much_left - 1;//I/O burst time = CPU burst time - 1
        }
    }

    printf("\n--FCFS--\n");
    fcfs(pinfos_copies[0], pinfos_len, io_burst);


    printf("\n--nonpreemptive SJF--\n");
    preemptive = 0;
    sjf(pinfos_copies[1], pinfos_len, preemptive);

    printf("\n--preemptive SJF--\n");
    preemptive = 1;
    sjf(pinfos_copies[2], pinfos_len, preemptive);

    printf("\n--nonpreemptive Priority--\n");
    preemptive = 0;
    prior(pinfos_copies[3], pinfos_len, preemptive);

    printf("\n--preemptive Priority--\n");
    preemptive = 1;
    prior(pinfos_copies[4], pinfos_len, preemptive);


    printf("Enter the number of time quantum: ");
    scanf("%d", &quant);

    printf("\n--Round Robin--\n");
    rr(pinfos_copies[5], pinfos_len, quant);

    printf("\n--Priority with Round Robin--\n");
    preemptive = 0;
    rr_prior(pinfos_copies[6], pinfos_len, quant);

    printf("\n--Highest Response Ratio Next--\n");
    hrr(pinfos_copies[7], pinfos_len);
    
    
    deallocate_mem(pinfos, pinfos_len);

    for (int i = 0; i < algo_num; ++i) {
        deallocate_mem(pinfos_copies[i], pinfos_len);
    }

}