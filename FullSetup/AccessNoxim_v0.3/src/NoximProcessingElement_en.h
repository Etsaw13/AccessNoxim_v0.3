#ifndef __NOXIMPROCESSINGELEMENT_H__
#define __NOXIMPROCESSINGELEMENT_H__

#include <queue>
#include <systemc.h>
#include <cmath>
#include <cassert>
#include "NoximMain.h"
#include "NoximGlobalTrafficTable.h"
using namespace std;

SC_MODULE(NoximProcessingElement_en)
{
    // I/O Ports
    sc_in_clk               clock;
    sc_in     < bool      > reset;

    sc_in     < NoximFlit > flit_rx;
    sc_in     < bool      > req_rx;
    sc_out    < bool      > ack_rx;

	sc_in     < NoximFlit > flit_semi_rx;
    sc_in     < bool      > req_semi_rx;
    sc_out    < bool      > ack_semi_rx;
	
    sc_out    < NoximFlit > flit_tx;
    sc_out    < bool      > req_tx;
    sc_in     < bool      > ack_tx;
	
	sc_out    < NoximFlit > flit_semi_tx;
    sc_out    < bool      > req_semi_tx;
    sc_in     < bool      > ack_semi_tx;

    sc_out    < int       > free_slots;
    sc_in     < int       > free_slots_neighbor;


    int                     local_id;
    int                     refly_pkt;
    bool                    _clean_all;
    bool                    transmittedAtPreviousCycle;
    int                     packet_queue_length;
	queue < NoximPacket >   message_queue;
	queue < NoximPacket >   packet_queue;
	queue < NoximFlit   >   flit_queue;
    
    void rxProcess();
    void txProcess();

    bool canShot(NoximPacket & packet );
    // void TraffThrottlingProcess();
	bool TLA(NoximPacket & packet );
	bool TAAR(NoximPacket & packet );
    NoximFlit nextFlit();
    NoximPacket trafficRandom();
    NoximPacket trafficTranspose1();
    int randInt(int min, int max);
    void fixRanges(const NoximCoord, NoximCoord &);
    
    SC_CTOR(NoximProcessingElement_en) {
        SC_METHOD(rxProcess);
        sensitive << reset;
        sensitive << clock.pos();

        SC_METHOD(txProcess);
        sensitive << reset;
        sensitive << clock.pos();
    }
};

#endif