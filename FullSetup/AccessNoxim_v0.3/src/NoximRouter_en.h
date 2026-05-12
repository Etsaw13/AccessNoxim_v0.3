#ifndef __NOXIMROUTER_EN_H__
#define __NOXIMROUTER_EN_H__

#include <systemc.h>
#include "NoximMain.h"
#include "NoximBuffer.h"
#include "NoximStats.h"
#include "NoximGlobalRoutingTable.h"
#include "NoximLocalRoutingTable.h"
#include "NoximReservationTable.h"
using namespace std;

SC_MODULE(NoximRouter_en){
    public:
    sc_in_clk                       clock;		                  				// The input clock for the router
    sc_in <bool>                    reset;                           			// The reset signal for the router

    sc_in  <NoximFlit>              flit_rx             [DIRECTIONS + 2];	  	// The input channels (including local one)
    sc_in  <bool     >              req_rx              [DIRECTIONS + 2];	  	// The requests associated with the input channels
    sc_out <bool     >              ack_rx              [DIRECTIONS + 2];	  	// The outgoing ack signals associated with the input channels
                                    
    sc_out <NoximFlit>              flit_tx             [DIRECTIONS + 2];   	// The output channels (including local one)
    sc_out <bool     >              req_tx              [DIRECTIONS + 2];	  	// The requests associated with the output channels
    sc_in  <bool     >              ack_tx              [DIRECTIONS + 2];	  	// The outgoing ack signals associated with the output channels

    sc_out <int>                    free_slots          [DIRECTIONS + 1];
    sc_in  <int>                    free_slots_neighbor [DIRECTIONS + 1];
	             
	sc_out <bool>                   on_off              [DIRECTIONS];		    // Information to neighbor router that if this router be throttled
	sc_in  <bool>                   on_off_neighbor     [DIRECTIONS];	        // Information of throttling that if neighbor router be throttled
	
	// sc_out <float>TB                [DIRECTIONS];				// Information to neighbor router TB 
	// sc_in  <float>TB_neighbor       [DIRECTIONS];	

	// sc_out <float>PDT               [DIRECTIONS];             // Information to neighbor router PDT
    // sc_in  <float>PDT_neighbor      [DIRECTIONS];

	// sc_out <float>buf[DIRECTIONS+2]             [DIRECTIONS];// Information to neighbor router buf0 
    // sc_in  <float>buf_neighbor[DIRECTIONS+2]    [DIRECTIONS];

    sc_out <int>                    free_slots_PE[4];

    int                             local_id;
    NoximBuffer                     buffer      [DIRECTIONS + 2];
    NoximStats                      stats;
    NoximLocalRoutingTable          routing_table;
    NoximReservationTable           reservation_table;
    int                             start_from_port;
    unsigned long                   routed_flits[DIRECTIONS + 2];
	unsigned long                   waiting	    [DIRECTIONS + 2];
    unsigned long                   _total_waiting;
	unsigned long                   routed_packets;
    bool 	                        _emergency;

    SC_CTOR(NoximRouter_en) {
	SC_METHOD(rxProcess);
	sensitive << reset;
	sensitive << clock.pos();

	SC_METHOD(txProcess);
	sensitive << reset;
	sensitive << clock.pos();

    SC_METHOD(bufferMonitor);
	sensitive << reset;
	sensitive << clock.pos();

    }

    // private:
    void rxProcess();
    void txProcess();
    void bufferMonitor();

    int route(const NoximRouteData & route_data);
    vector<int> routingFunction(const NoximRouteData & route_data);
    int selectionFunction(const vector <int> &directions, const NoximRouteData & route_data);
    int selectionRandom(const vector < int >&directions);

    vector < int >routingWestFirst_yly   (const NoximCoord & current                          ,const NoximCoord & destination);
};
#endif