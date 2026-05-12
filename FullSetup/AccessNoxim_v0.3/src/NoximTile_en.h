#ifndef __NOXIMTILE_EN_H__
#define __NOXIMTILE_EN_H__

#include <systemc.h>
#include "NoximRouter_en.h"
#include "NoximProcessingElement_en.h"
using namespace std;

SC_MODULE(NoximTile)
{
    // I/O Ports
    sc_in_clk                       clock;		                // The input clock for the tile
    sc_in <bool>                    reset;	                        // The reset signal for the tile

    sc_in  <NoximFlit>              flit_rx             [DIRECTIONS];	// The input channels
    sc_in  <bool>                   req_rx              [DIRECTIONS];	        // The requests associated with the input channels
    sc_out <bool>                   ack_rx              [DIRECTIONS];	        // The outgoing ack signals associated with the input channels

    sc_out <NoximFlit>              flit_tx             [DIRECTIONS];	// The output channels
    sc_out <bool>                   req_tx              [DIRECTIONS];	        // The requests associated with the output channels
    sc_in  <bool>                   ack_tx              [DIRECTIONS];	        // The outgoing ack signals associated with the output channels

    sc_out <int>                    free_slots          [DIRECTIONS];
    sc_in  <int>                    free_slots_neighbor [DIRECTIONS];

	sc_out<bool>	                on_off              [DIRECTIONS];
	sc_in<bool>		                on_off_neighbor     [DIRECTIONS];


    // Signals
    sc_signal <NoximFlit>           flit_rx_local;	    // The input channels	
    sc_signal <bool>                req_rx_local;        // The requests associated with the input channels
    sc_signal <bool>                ack_rx_local;	    // The outgoing ack signals associated with the input channels
	
	sc_signal <NoximFlit>           flit_rx_semi_local;	// The input channels
	sc_signal <bool>                req_rx_semi_local;   // The requests associated with the input channels
	sc_signal <bool>                ack_rx_semi_local;	// The outgoing ack signals associated with the input channels
	
    sc_signal <NoximFlit>           flit_tx_local;	    // The output channels
    sc_signal <bool>                req_tx_local;	    // The requests associated with the output channels
    sc_signal <bool>                ack_tx_local;	    // The outgoing ack signals associated with the output channels

	sc_signal <NoximFlit>           flit_tx_semi_local;	// The output channels
	sc_signal <bool>                req_tx_semi_local;	// The requests associated with the output channels
	sc_signal <bool>                ack_tx_semi_local;	// The outgoing ack signals associated with the output channels
	
    sc_signal <int>                 free_slots_local;
    sc_signal <int>                 free_slots_neighbor_local;

	// sc_signal <int>                 free_slots_neighbor_router[4];
	// Instances
    NoximRouter_en                  *r;		                // Router instance
    NoximProcessingElement_en       *pe;	                // Processing Element instance

    // Constructor
	int i;
    SC_CTOR(NoximTile) {
        r = new NoximRouter_en("Router");
        r->clock(clock);
        r->reset(reset);

        for ( i = 0; i < DIRECTIONS  ; i++) {
            r->flit_rx[i] (flit_rx[i]);
            r->req_rx[i] (req_rx[i]);
            r->ack_rx[i] (ack_rx[i]);

            r->flit_tx[i] (flit_tx[i]);
            r->req_tx[i] (req_tx[i]);
            r->ack_tx[i] (ack_tx[i]);

            r->free_slots[i] (free_slots[i]);
            r->free_slots_neighbor[i] (free_slots_neighbor[i]);
            
            r->on_off[i](on_off[i]);
            r->on_off_neighbor[i](on_off_neighbor[i]);
        }

        r->flit_rx[DIRECTION_LOCAL     ] (flit_tx_local     );
        r->req_rx [DIRECTION_LOCAL     ] (req_tx_local      );
        r->ack_rx [DIRECTION_LOCAL     ] (ack_tx_local      );
                                                            
        r->flit_tx[DIRECTION_LOCAL     ] (flit_rx_local     );
        r->req_tx [DIRECTION_LOCAL     ] (req_rx_local      );
        r->ack_tx [DIRECTION_LOCAL     ] (ack_rx_local      );

        r->flit_rx[DIRECTION_SEMI_LOCAL] (flit_tx_semi_local);
        r->req_rx [DIRECTION_SEMI_LOCAL] (req_tx_semi_local );
        r->ack_rx [DIRECTION_SEMI_LOCAL] (ack_tx_semi_local );
                            
        r->flit_tx[DIRECTION_SEMI_LOCAL] (flit_rx_semi_local);
        r->req_tx [DIRECTION_SEMI_LOCAL] (req_rx_semi_local );
        r->ack_tx [DIRECTION_SEMI_LOCAL] (ack_rx_semi_local );
        
        r->free_slots         [DIRECTION_LOCAL](free_slots_local);
        r->free_slots_neighbor[DIRECTION_LOCAL](free_slots_neighbor_local);
        

        pe = new NoximProcessingElement_en("ProcessingElement");
        // pe->clock(clock);
        // pe->reset(reset);

        // pe->flit_rx(flit_rx_local);
        // pe->req_rx(req_rx_local);
        // pe->ack_rx(ack_rx_local);

        // pe->flit_semi_rx(flit_rx_semi_local);
        // pe->req_semi_rx(req_rx_semi_local);
        // pe->ack_semi_rx(ack_rx_semi_local);
        
        // pe->flit_tx(flit_tx_local);
        // pe->req_tx(req_tx_local);
        // pe->ack_tx(ack_tx_local);
        
        // pe->flit_semi_tx(flit_tx_semi_local);
        // pe->req_semi_tx(req_tx_semi_local);
        // pe->ack_semi_tx(ack_tx_semi_local);
        
        // pe->free_slots(free_slots_neighbor_local);
        // pe->free_slots_neighbor(free_slots_local);

        
        
        // for( i = 0 ; i < 4 ; i++){
        // r->free_slots_PE[i](free_slots_neighbor_router[i]);
        // pe->free_slots_router[i](free_slots_neighbor_router[i]);
        // }
        // for( i = 0 ; i < 8 ; i++){
        //     r ->RCA_PE[i](RCA_PE_router[i]);
        //     pe->RCA_router[i](RCA_PE_router[i]);
        // }
        // for( i = 0 ; i < 4 ; i++){
        //     r->NoP_PE[i](NoP_PE_router[i]);
        //     pe->NoP_router[i](NoP_PE_router[i]);
        // }
    }
};

#endif