#include <math.h>
#include "NoximProcessingElement.h"

void NoximProcessingElement_en::rxProcess(){
    if (reset.read()) {
		_round_MC = 0;
		ack_rx.write(1);
		ack_semi_rx.write(1);
		_adaptive_transmit              =0;
		_dor_transmit                   =0;
		_dw_transmit                    =0;
		_mid_adaptive_transmit          =0;
		_mid_dor_transmit               =0;
		_mid_dw_transmit                =0;
		_beltway_transmit               =0;
		_Transient_adaptive_transmit    =0; 
		_Transient_dor_transmit         =0; 
		_Transient_dw_transmit          =0; 
		_Transient_mid_adaptive_transmit=0; 
		_Transient_mid_dor_transmit     =0;
		_Transient_mid_dw_transmit      =0;
		_Transient_beltway_transmit 	=0;	
		refly_pkt            = 0;
		while( !flit_queue.empty() ) 
			flit_queue.pop();
		/*DFS = 4;
  		RST = 0;*/
    }
	else{
		if( req_rx.read()==1 && ack_rx.read() == 1 ){
			NoximFlit flit_tmp = flit_rx.read();
			if (NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
				cout << sc_simulation_time() << ": ProcessingElement[" <<
				local_id << "] RECEIVING " << flit_tmp << endl;
			}
			_flit_static(flit_tmp);//statics
		}
		if( req_semi_rx.read()==1 && ack_semi_rx.read() == 1 ){
			NoximFlit flit = flit_semi_rx.read(); //Jimmy modified on 2011.10.25
			if(flit.flit_type == FLIT_TYPE_TAIL) refly_pkt++;
			if (NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
				cout << sc_simulation_time() << ": ProcessingElement[" <<
					local_id << "] RECEIVING " << flit << endl;
			}
			flit_queue.push(flit); //Jimmy modified on 2011.05.29
		}
		ack_rx.write( !_emergency );
		// ack_semi_rx.write( !_emergency && ( refly_pkt < DEFAULT_PACKET_QUEUE_LENGTH ) );
		ack_semi_rx.write( !_emergency );
    }
}