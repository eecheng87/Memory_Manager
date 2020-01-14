#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#define DISK_SIZE 1000
#define PFI_DBI 0
#define INUSE 1
#define PRESENT 2
#define REF 3
#define DIRTY 4
#define IN_ACT 5
int main(int argc,char* argv[])
{
    char buffer[1024];
    char policy[10];
    int line_cnt = 0;
    // virtual page number
    int vpn = 0;
    // physical frame number
    int pfn = 0;
    /*
    CRITICAL:  sometimes pfn will not be assigned ,
    FIX later!!!!!!!!!!!!!!!
    */
    char *del = " ";
    char *pch;
    char *lf;

    // ex: write 2
    int write; // 1 means true; 0 means false
    int vpi;

    while(fgets(buffer,sizeof(buffer),stdin))
    {
        if(line_cnt == 0)
        {
            char *lf;
            pch = strtok(buffer,del);
            pch = strtok(NULL,del);
            // remove change line char
            if((lf = strchr(pch,'\n'))!=NULL)
                *lf = '\0';
            strcpy(policy,pch);
        }
        else if(line_cnt == 1)
        {
            pch = strtok(buffer,del);
            pch = strtok(NULL,del);
            pch = strtok(NULL,del);
            pch = strtok(NULL,del);
            pch = strtok(NULL,del);
            if((lf = strchr(pch,'\n'))!=NULL)
                *lf = '\0';
            vpn = atoi(pch);
        }
        else if(line_cnt == 2)
        {
            pch = strtok(buffer,del);
            pch = strtok(NULL,del);
            pch = strtok(NULL,del);
            pch = strtok(NULL,del);
            pch = strtok(NULL,del);
            if((lf = strchr(pch,'\n'))!=NULL)
                *lf = '\0';
            pfn = atoi(pch);
            //printf("%d\n",pfn);
            break;
        }
        line_cnt++;
    }

    // declare page table, virtual mem, phy mem.
    //int virtual_mem[vpn];
    int physical_mem[pfn];
    int page_table[vpn][6];
    int disk[DISK_SIZE];

    // policy parameter
    int fifo_ptr = 0;
    int esca_ptr = 0;
    // miss rate parameter
    int miss_cnt = 0;
    int total_cnt = 0;

    // declare data structure for slru policy
    int act_size = pfn%2==0?pfn/2:(pfn-1)/2;
    int inact_size = pfn%2==0?pfn/2:(pfn+1)/2;
    int act_li[act_size];
    int inact_li[inact_size];
    int act_tail = -1; // index for last element in active list
    int inact_tail = -1; // index for last element in inactive list
    int tar_index;
    //printf("%d %d\n",act_size,inact_size);
    // init
    int i;
    for(i = 0; i < act_size; ++i)
    {
        act_li[i] = -1;
    }
    for(i = 0; i < inact_size; ++i)
    {
        inact_li[i] = -1;
    }
    for(i = 0; i < pfn; ++i)
    {
        physical_mem[i] = -1;
    }
    for(i = 0; i < vpn; ++i)
    {
        // virtual_mem[i] = -1;
        page_table[i][PFI_DBI] = -1; // haven't been reference
        page_table[i][INUSE] = 0; // reset in-use bit
        page_table[i][PRESENT] = 0; // reset present bit
        page_table[i][REF] = 0;
        page_table[i][DIRTY] = 0;
        page_table[i][IN_ACT] = -1; // negative value means not in both active and inact
        // '1' means in active list
        // '0' means in inactive list
    }
    for(i = 0; i < DISK_SIZE; ++i)
    {
        disk[i] = -1; // free
    }

    // eat ------trace--------
    if(fgets(buffer,sizeof(buffer),stdin)==NULL)
    {
        printf("Something is wrong\n");
    }
    while(fgets(buffer,sizeof(buffer),stdin))
    {
        //line_cnt++;
        //if(line_cnt >= 4){
        total_cnt++;
        pch = strtok(buffer,del);
        write = strcmp(pch,"Write") == 0?1:0;
        pch = strtok(NULL,del);

        if((lf = strchr(pch,'\n'))!=NULL)
            *lf = '\0';

        vpi = atoi(pch);
        // complete collecting info
        // write = 1 means its behavior is writing
        // vpi is VPI

        if(page_table[vpi][PRESENT] == 1)
        {
            // data is already in phy mem
            printf("Hit, %d=>%d\n",vpi,page_table[vpi][PFI_DBI]);
            // maybe need to do something for other replace policy
            /*
                maybe need to update configure bit in pt
            */
            // page_table[vpi][REF] = 1;
            // maybe need to change to
            page_table[vpi][REF] = strcmp(policy,"SLRU")==0?page_table[vpi][REF]:1;
            // modified: original
            // page_table[vpi][DIRTY] = write==1?1:0;
            page_table[vpi][DIRTY] = write==1?1:page_table[vpi][DIRTY];

            // if it is SLRU policy, need to update list
            if(strcmp(policy,"SLRU") == 0)
            {

                if(page_table[vpi][IN_ACT] == 0)
                {
                    // page hit occur in inactive list
                    // printf("%d %d\n",vpi,page_table[vpi][REF]);
                    if(page_table[vpi][REF] == 0)
                    {
                        // if the page has reference bit = 0
                        // set reference bit and move the page to inactive list head

                        page_table[vpi][REF] = 1;
                        tar_index = 0;
                        for(i = 0; i < inact_size; ++i)
                        {
                            if(inact_li[i] == vpi)
                            {
                                tar_index = i;
                                i = inact_size;
                            }
                        }
                        if(tar_index!=0)
                        {
                            // if tar_index is 0, there is no need to re-arrange
                            for(i = tar_index; i > 0; --i)
                            {
                                inact_li[i] = inact_li[i-1];
                            }
                            inact_li[0] = vpi;
                        }
                    }
                    else if(act_size!=0)
                    {
                        // if the page has reference bit = 1
                        // clear reference bit and move the page to active list head
                        page_table[vpi][REF] = 0;
                        if(act_tail == act_size-1)
                        {
                            // active list is full
                            // Search the tail of the active list for evicted page
                            int tmp = act_tail;
                            while(page_table[act_li[tmp]][REF]==1)
                            {
                                // if the page has reference bit = 1
                                // clear reference bit and move the page to active list head
                                // search the tail of the active list for another evicted page
                                page_table[act_li[tmp]][REF] = 0;
                                int tmp_vpi = act_li[tmp];
                                for(i = act_tail; i > 0; --i)
                                {
                                    act_li[i] = act_li[i-1];
                                }
                                act_li[0] = tmp_vpi;
                            }
                            // act_li[tmp]'s ref must be 0 now
                            // if the page in active list has reference bit = 0
                            // refill the page to inactive list head
                            int act_vic = act_li[tmp];
                            tar_index = 0;
                            for(i = 0; i < inact_size; ++i)
                            {
                                if(inact_li[i] == vpi)
                                {
                                    tar_index = i;
                                    i = inact_size;
                                }
                            }
                            // left shift list, let head be empty
                            for(i = tar_index; i > 0; --i)
                            {
                                inact_li[i] = inact_li[i-1];
                            }
                            // refill the page tp inactive list head
                            inact_li[0] = act_vic;
                            // move the page to active list head
                            // (1) right shift list
                            for(i = act_size-1; i > 0; --i)
                            {
                                act_li[i] = act_li[i-1];
                            }
                            // (2) move the page to active list head
                            act_li[0] = vpi;
                            // update page table info
                            page_table[vpi][IN_ACT] = 1;
                            page_table[act_vic][IN_ACT] = 0;
                        }
                        else
                        {
                            // active list still has free space
                            // move the page to active list head
                            act_tail++;
                            for(i = act_tail; i > 0; --i)
                            {
                                act_li[i] = act_li[i-1];
                            }
                            act_li[0] = vpi;
                            // update page table info
                            page_table[vpi][IN_ACT] = 1;
                            // reomve page from inactive list
                            tar_index = 0;
                            for(i = 0; i < inact_size; ++i)
                            {
                                if(inact_li[i] == vpi)
                                {
                                    tar_index = i;
                                    i = inact_size;
                                }
                            }
                            for(i = tar_index; i < inact_tail; ++i)
                            {
                                inact_li[i] = inact_li[i+1];
                            }
                            inact_tail--;
                        }
                    }
                }
                else if(page_table[vpi][IN_ACT] == 1)
                {
                    // page hit occur in active list
                    if(page_table[vpi][REF] == 0)
                    {
                        // if the page has reference bit = 0
                        // set reference bit and move the page to active list head
                        page_table[vpi][REF] = 1;
                    }
                    // move the page to active list head
                    tar_index = 0;
                    for(i = 0; i < act_size; ++i)
                    {
                        if(act_li[i] == vpi)
                        {
                            tar_index = i;
                            i = act_size;
                        }
                    }
                    if(tar_index!=0)
                    {
                        // if tar_index is 0, there is no need to re-arrange
                        for(i = tar_index; i > 0; --i)
                        {
                            act_li[i] = act_li[i-1];
                        }
                        act_li[0] = vpi;
                    }
                }
            }
        }
        else
        {
            // data isn't in phy mem
            miss_cnt++;
            int phy_full = 1;
            int in_disk = -1;
            int src;
            int dest;
            int src_vpi;
            int dest_vpi;
            int out_pfi;
            int victim_phy_index = 0;
            // check whether phy_mem is full

            for(i = 0; i < pfn; ++i)
            {
                if(physical_mem[i] < 0)
                {
                    phy_full = 0; // false
                    i = pfn;
                }
            }
            // check whether target vpi is in disk
            in_disk = -1;
            for(i = 0; i < DISK_SIZE; ++i)
            {
                if(disk[i] == vpi)
                {
                    // vpi is in disk
                    in_disk = i;
                    i = DISK_SIZE;
                }
            }

            if((phy_full == 1)||((strcmp(policy,"SLRU") == 0)&&inact_tail==inact_size-1))
            {
                // must kick someone in phy_mem to disk
                if(strcmp(policy,"FIFO") == 0)
                {
                    fifo_ptr = fifo_ptr%pfn;
                    // find free space in disk, the place store evicted data
                    for(i = 0; i < DISK_SIZE; ++i)
                    {
                        if(disk[i] < 0)
                        {
                            dest = i;
                            disk[i] = physical_mem[fifo_ptr];
                            dest_vpi = disk[i];
                            page_table[physical_mem[fifo_ptr]][PRESENT] = 0;
                            //page_table[physical_mem[fifo_ptr]][INUSE]
                            page_table[physical_mem[fifo_ptr]][PFI_DBI] = dest;
                            i = DISK_SIZE;
                        }
                    }
                    if(in_disk >= 0)
                    {
                        // target is already in disk
                        src = in_disk;
                        src_vpi = vpi;
                    }
                    else
                    {
                        src = -1;
                        src_vpi = vpi;
                    }
                    page_table[src_vpi][PRESENT] = 1;
                    //page_table[src_vpi][INUSE] = 1;
                    page_table[src_vpi][PFI_DBI] = fifo_ptr;
                    // free disk
                    disk[in_disk] = -1;
                    out_pfi = fifo_ptr;
                    physical_mem[fifo_ptr] = src_vpi;
                    fifo_ptr++;
                    //fifo_ptr = (fifo_ptr!=-1)?fifo_ptr+1:0;
                }
                else if(strcmp(policy,"ESCA") == 0)
                {
                    // deciding victim
                    int victim_vpi = -1;
                    int mov_cnt = 0;
                    while(victim_vpi < 0)
                    {
                        // a) looking for (0,0)
                        while(mov_cnt!=pfn)
                        {
                            esca_ptr = esca_ptr%pfn;
                            //printf("(1)=> %d\n",esca_ptr);
                            if(page_table[physical_mem[esca_ptr]][PRESENT] == 1)
                            {
                                if((page_table[physical_mem[esca_ptr]][REF] == 0)&&(page_table[physical_mem[esca_ptr]][DIRTY] == 0))
                                {
                                    victim_vpi = physical_mem[esca_ptr];
                                    esca_ptr++;
                                    break;
                                }
                            }
                            esca_ptr++;
                            mov_cnt++;
                        }
                        mov_cnt = 0;
                        if(victim_vpi >= 0)
                            break;
                        /*for(i = 0; i < pfn; ++i)
                        {
                            if(page_table[physical_mem[i]][PRESENT] == 1)
                            {
                                //printf("=> %d %d %d\n",i,page_table[i][REF],page_table[i][DIRTY]);
                                if((page_table[physical_mem[i]][REF] == 0)&&(page_table[physical_mem[i]][DIRTY] == 0))
                                {
                                    //printf("%d\n",i);
                                    victim_vpi = physical_mem[i];
                                    break;
                                }
                            }
                        }*/
                        // keep find victim in method b
                        // b) looking for (0,1)
                        while(mov_cnt!=pfn)
                        {
                            esca_ptr = esca_ptr%pfn;
                            //printf("(2)=> %d\n",esca_ptr);
                            if(page_table[physical_mem[esca_ptr]][PRESENT] == 1)
                            {
                                if((page_table[physical_mem[esca_ptr]][REF] == 0 )&&(page_table[physical_mem[esca_ptr]][DIRTY] == 1))
                                {
                                    victim_vpi = physical_mem[esca_ptr];
                                    esca_ptr++;
                                    break;
                                }
                                else
                                {
                                    page_table[physical_mem[esca_ptr]][REF] = 0;
                                }
                            }
                            esca_ptr++;
                            mov_cnt++;
                        }
                        mov_cnt = 0;
                        /*if(victim_vpi < 0)
                        {
                            for(i = 0; i < pfn; ++i)
                            {
                                if(page_table[physical_mem[i]][PRESENT] == 1)
                                {
                                    if((page_table[physical_mem[i]][REF] == 0 )&&(page_table[physical_mem[i]][DIRTY] == 1))
                                    {
                                        victim_vpi = physical_mem[i];
                                        break;
                                    }
                                }
                            }
                        }*/
                        // c) clear ref bit
                        /*if(victim_vpi < 0)
                        {
                            for(i = 0; i < vpn; ++i)
                            {
                                //printf("XX %d %d %d %d\n",i,page_table[i][PRESENT],page_table[i][REF],page_table[i][DIRTY]);
                                if(page_table[i][PRESENT] == 1)
                                {
                                    page_table[i][REF] = 0;
                                }
                                //printf("OO %d %d %d %d\n",i,page_table[i][PRESENT],page_table[i][REF],page_table[i][DIRTY]);
                            }
                        }*/
                    }
                    // get `victim_vpi` which will be evicted
                    // find free space in disk, the place store evicted data
                    for(i = 0; i < DISK_SIZE; ++i)
                    {
                        if(disk[i] < 0)
                        {
                            victim_phy_index = page_table[victim_vpi][PFI_DBI];
                            dest = i;
                            disk[i] = victim_vpi;
                            dest_vpi = disk[i];
                            page_table[victim_vpi][PRESENT] = 0;
                            //page_table[physical_mem[fifo_ptr]][INUSE]
                            page_table[victim_vpi][PFI_DBI] = dest;
                            i = DISK_SIZE;
                        }
                    }
                    if(in_disk >= 0)
                    {
                        // target is already in disk
                        src = in_disk;
                        src_vpi = vpi;
                    }
                    else
                    {
                        src = -1;
                        src_vpi = vpi;
                    }
                    page_table[src_vpi][PRESENT] = 1;
                    page_table[src_vpi][REF] = 1;
                    page_table[src_vpi][DIRTY] = write==1?1:0;
                    //page_table[src_vpi][INUSE] = 1;
                    page_table[src_vpi][PFI_DBI] = victim_phy_index;
                    // free disk
                    disk[in_disk] = -1;
                    physical_mem[victim_phy_index] = src_vpi;
                    out_pfi = victim_phy_index;
                }
                else if(strcmp(policy,"SLRU") == 0)
                {
                    // in-active list is full
                    // Search the tail of the inactive list for evicted page
                    // if the page has reference bit = 0
                    // swapped out the page
                    // if the page has reference bit = 1
                    // clear reference bit and move the page to inactive list head
                    // search the tail of the inactive list for another evicted page
                    //printf("hi\n");
                    int inactv = 0;
                    int vic_phy_index = 0;
                    while(page_table[inact_li[inact_tail]][REF]==1)
                    {
                        page_table[inact_li[inact_tail]][REF] = 0;
                        inactv = inact_li[inact_tail];
                        for(i =inact_tail; i > 0; --i)
                        {
                            inact_li[i] = inact_li[i-1];
                        }
                        inact_li[0] = inactv;
                    }
                    // REF of last element in in-active list must be 0
                    // evict it
                    vic_phy_index = page_table[inact_li[inact_tail]][PFI_DBI];

                    // (1) find free space in disk, the place store evicted data
                    for(i = 0; i < DISK_SIZE; ++i)
                    {
                        if(disk[i] < 0)
                        {
                            dest = i;
                            disk[i] = inact_li[inact_tail];
                            dest_vpi = disk[i];
                            page_table[inact_li[inact_tail]][PRESENT] = 0;
                            //page_table[physical_mem[fifo_ptr]][INUSE]
                            page_table[inact_li[inact_tail]][PFI_DBI] = dest;
                            page_table[inact_li[inact_tail]][IN_ACT] = -1;
                            i = DISK_SIZE;
                        }
                    }
                    // (2)
                    if(in_disk >= 0)
                    {
                        // target is already in disk
                        src = in_disk;
                        src_vpi = vpi;
                    }
                    else
                    {
                        src = -1;
                        src_vpi = vpi;
                    }
                    page_table[src_vpi][PRESENT] = 1;
                    //page_table[src_vpi][INUSE] = 1;
                    page_table[src_vpi][PFI_DBI] = vic_phy_index;
                    page_table[src_vpi][REF] = 1;
                    page_table[src_vpi][IN_ACT] = 0;
                    physical_mem[vic_phy_index] = vpi;

                    // free disk
                    disk[in_disk] = -1;
                    out_pfi = vic_phy_index;
                    // kick vic from in-active list and put ref page in head
                    for(i = inact_tail; i > 0; --i)
                    {
                        inact_li[i] = inact_li[i-1];
                    }
                    inact_li[0] = src_vpi;
                    //printf("%d\n",out_pfi);
                    //printf("%d\n",inact_tail);
                }

            }
            else
            {
                int slru_src = -1;
                if(strcmp(policy,"SLRU") == 0)
                {
                    // in-active list isn't full yet
                    // vpi not present in physical mem
                    // swap page into head of in-active list
                    //printf("test %d %d",inact_li[0],inact_li[1]);
                    for(i = inact_tail+1; i > 0; --i)
                    {
                        //printf("$ %d",inact_li[i]);
                        inact_li[i] = inact_li[i-1];
                    }
                    page_table[vpi][IN_ACT] = 0;
                    inact_li[0] = vpi;
                    inact_tail++;
                    // check whether vpi already in disk
                    if(in_disk>=0)
                    {
                        slru_src = in_disk;
                    }
                    // free vpi in disk
                    disk[in_disk] = -1;
                }
                //else{
                i = 0;
                while(i < pfn)
                {
                    if(physical_mem[i] == -1)
                    {
                        // entry `i` is empty
                        // there are not anyone be evicted
                        // disk must not exist `vpi`
                        src = strcmp(policy,"SLRU")==0?slru_src:-1;
                        dest = -1;
                        dest_vpi = -1;
                        src_vpi = vpi;
                        physical_mem[i] = vpi;
                        out_pfi = i;
                        page_table[vpi][PFI_DBI] = i;
                        //page_table[vpi][INUSE]
                        page_table[vpi][PRESENT] = 1;
                        // needed for esca policy
                        page_table[vpi][REF] = 1;
                        page_table[vpi][DIRTY] = write==1?1:0;
                        break;
                    }
                    ++i;
                }
                // }
            }
            printf("Miss, %d, %d>>%d, %d<<%d\n",out_pfi,dest_vpi,dest,src_vpi,src);
        }
        //}
        //memset(buffer,0,sizeof(buffer));
        //printf("hi %d\n",inact_tail);
        /*printf("physical memory(ref,dirty): ");
        for(i = 0; i < pfn; ++i){
            printf("%d(%d,%d) ",physical_mem[i],page_table[physical_mem[i]][REF],page_table[physical_mem[i]][DIRTY]);
            //printf("%d ",physical_mem[i]);
        }

        printf("\n");
        printf("in-active list:");
        for(i = 0; i <= inact_tail;++i){
            printf("%d(%d) ",inact_li[i],page_table[inact_li[i]][REF]);
        }
        printf("\n");
        printf("active list:");
        for(i = 0; i <= act_tail;++i){
            printf("%d(%d) ",act_li[i],page_table[act_li[i]][REF]);
        }
        printf("\n\n");  */
        // printf("\n\n");
    }
    printf("Page Fault Rate: %.3f\n",(float)miss_cnt/total_cnt);
    return 0;
}