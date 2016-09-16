//
// Created by alihitawala on 9/11/16.
//

#ifndef UDPECHO_MASTER_PACKET_H
#define UDPECHO_MASTER_PACKET_H

struct Packet {
    int seq_num;
    int length;
    char data[255];
};

#endif //UDPECHO_MASTER_PACKET_H
