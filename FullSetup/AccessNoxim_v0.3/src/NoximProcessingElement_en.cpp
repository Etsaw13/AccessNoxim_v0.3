#include <math.h>
#include "NoximProcessingElement_en.h"

void NoximProcessingElement_en::rxProcess(){
    if (reset.read()) {
		ack_rx.write(1);
		ack_semi_rx.write(1);
		refly_pkt = 0;
		while( !flit_queue.empty() ) 
			flit_queue.pop();
    }
	else{
		if( req_rx.read()==1 && ack_rx.read() == 1 ){
			NoximFlit flit_tmp = flit_rx.read();
			if (NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
				cout << sc_simulation_time() << ": ProcessingElement[" <<
				local_id << "] RECEIVING " << flit_tmp << endl;
			}
			// _flit_static(flit_tmp);
		}
		if( req_semi_rx.read()==1 && ack_semi_rx.read() == 1 ){
			NoximFlit flit = flit_semi_rx.read();
			if(flit.flit_type == FLIT_TYPE_TAIL) refly_pkt++;
			if (NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
				cout << sc_simulation_time() << ": ProcessingElement[" <<
					local_id << "] RECEIVING " << flit << endl;
			}
			flit_queue.push(flit);
		}
		// ack_rx.write( !_emergency );
		// ack_semi_rx.write( !_emergency );
		ack_rx.write( 1 );
		ack_semi_rx.write( 1 );
    }
}

void NoximProcessingElement_en::txProcess(){
    if (reset.read()){
		req_tx.write(0);
		req_semi_tx.write(0);
		while( !packet_queue.empty() ) 
			packet_queue.pop();
		while( !message_queue.empty() )
                        message_queue.pop();
		transmittedAtPreviousCycle = false;
		_clean_all = false;
		packet_queue_length = 0;
    } 
	
	else{
		NoximPacket  packet;
		NoximFlit    flit;
		if( !_clean_all ){
			if ( canShot(packet) ){
				message_queue.push(packet);
				transmittedAtPreviousCycle = true;
			} 
			else{
				transmittedAtPreviousCycle = false;
			}
		}
		if (packet_queue_length < DEFAULT_PACKET_QUEUE_LENGTH && !message_queue.empty() && !_clean_all){
			packet = message_queue.front();
			if ( TLA(packet) ){ // TODO: simplify
				// if( NoximGlobalParams::routing_algorithm > 10 || packet.routing != ROUTING_DOWNWARD_CROSS_LAYER){
					NoximCoord packet_dst = id2Coord(packet.dst_id);
					NoximCoord packet_src = id2Coord(packet.src_id);
					// if(throttling[packet_dst.x][packet_dst.y][packet_dst.z] == 0 && throttling[packet_src.x][packet_src.y][packet_src.z] == 0){
						TAAR(packet); // TODO: simplify
						packet.timestamp_ni = getCurrentCycleNum();
						packet_queue.push( packet );
						message_queue.pop();
						packet_queue_length++;
					// }
				// }
			}
		}
	
		 if( _clean_all ){
			 if ( !flit_queue.empty() ){
				 flit = flit_queue.front();
				 if(flit.flit_type==FLIT_TYPE_HEAD &&  refly_pkt > 0){
					 while( refly_pkt > 0 && NoximGlobalParams::message_level){
						if (flit.flit_type==FLIT_TYPE_TAIL)
							refly_pkt--;
						flit_queue.pop();
						if( flit_queue.empty() )
							break;
						else
							flit = flit_queue.front();
					 }
				 }
			 }
			while(!message_queue.empty()){
				message_queue.pop();	
			}
		 }
		
		if(ack_tx.read() == 1){
			req_tx.write(0);
			if (!packet_queue.empty()){
				flit = nextFlit();
				if( flit.dst_id != NOT_VALID){
					if (NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
						cout << getCurrentCycleNum() << ": ProcessingElement[" << local_id <<
						"] SENDING " << flit << endl;
					}
					flit_tx->write(flit);
					req_tx.write(1);
				}
			}	
		}
                
		if( ack_semi_tx.read() == 1){
			req_semi_tx.write(0);
			if( !flit_queue.empty() ){
			    if( refly_pkt > 0 ){
					flit = flit_queue.front();
					NoximPacket p;
					assert( flit.mid_id < MAX_ID + 1);
					assert( flit.dst_id < MAX_ID + 1);
					p.src_id = flit.mid_id;
					p.dst_id = flit.dst_id;
					while(!TLA(p)){
						if( flit.flit_type == FLIT_TYPE_HEAD ){
							NoximCoord flit_dst = id2Coord(flit.dst_id);
							if ( throttling[flit_dst.x][flit_dst.y][flit_dst.z] )cout<< getCurrentCycleNum() << ":Packet drop.(dst)\t"<<"refly pkt: "<<refly_pkt<<endl;
							else cout<< getCurrentCycleNum() << ":"<<flit<<" drop."<<endl;
						}
						flit_queue.pop();
						if ( flit.flit_type == FLIT_TYPE_TAIL) refly_pkt--;
						if ( refly_pkt == 0)break;
						flit = flit_queue.front();
						p.src_id = flit.mid_id;
						p.dst_id = flit.dst_id;
					}
				}
				if( refly_pkt > 0 ){
					flit = flit_queue.front();
					NoximPacket p;
					assert( flit.mid_id < MAX_ID + 1);
					assert( flit.dst_id < MAX_ID + 1);
					p.src_id = flit.mid_id;
					p.dst_id = flit.dst_id;
					if( !TLA(p)){
						cout<<getCurrentCycleNum()<<":"<<flit<<endl;
						cout<<"flit.current_id"<<"="<<local_id       <<id2Coord(local_id   )<<endl; 
						cout<<"flit.src_id    "<<"="<<flit.src_id    <<id2Coord(flit.src_id)<<endl;
						cout<<"flit.mid_id    "<<"="<<flit.mid_id    <<id2Coord(flit.mid_id)<<endl; 		
						cout<<"flit.dst_id    "<<"="<<flit.dst_id    <<id2Coord(flit.dst_id)<<endl; 
						cout<<"flit.routing   "<<"="<<flit.routing_f <<endl; 
						cout<<"flit.DW_layer  "<<"="<<flit.DW_layer  <<endl; 
						cout<<"flit.arr_mid   "<<"="<<flit.arr_mid   <<endl; 
						cout<<"refly pkt: "<<refly_pkt<<endl;
						assert(0);
					}

					flit.arr_mid   = true;
					if( flit.beltway && NoximGlobalParams::beltway && p.routing != ROUTING_DOWNWARD_CROSS_LAYER )
						flit.routing_f = ROUTING_WEST_FIRST; //20130923
					else 
						flit.routing_f = p.routing;
					flit_semi_tx->write(flit);
					req_semi_tx ->write(1);
					if ( flit.flit_type == FLIT_TYPE_TAIL) refly_pkt--;
					flit_queue.pop();
					if ( NoximGlobalParams::verbose_mode > VERBOSE_OFF) {
						cout << getCurrentCycleNum() << ": ProcessingElement[" << local_id <<
						"] SENDING " << flit << endl;
					}
				}
			}
		}
    }
}

bool NoximProcessingElement_en::canShot(NoximPacket & packet){
    bool shot;
    double threshold;

    if (NoximGlobalParams::traffic_distribution != TRAFFIC_TABLE_BASED){
		if (!transmittedAtPreviousCycle)
			threshold = NoximGlobalParams::packet_injection_rate;
		else
			threshold = NoximGlobalParams::probability_of_retransmission;
		
		shot = (((double) rand()) / RAND_MAX < threshold);
		if (shot) {
			switch (NoximGlobalParams::traffic_distribution) {
			case TRAFFIC_RANDOM:
			packet = trafficRandom();
			break;
	
			case TRAFFIC_TRANSPOSE1:
			packet = trafficTranspose1(); // use this first
			break;

			// case TRAFFIC_TRANSPOSE2:
			// packet = trafficTranspose2();
			// break;
			// case TRAFFIC_BIT_REVERSAL:
			// packet = trafficBitReversal();
			// break;
			// case TRAFFIC_SHUFFLE:
			// packet = trafficShuffle();
			// break;
			// case TRAFFIC_BUTTERFLY:
			// packet = trafficButterfly();
			// break;
	
			default:
			assert(false);
			}
		}
    }
	else{
		assert(false);
    }
	
    return shot;
}

NoximPacket NoximProcessingElement_en::trafficRandom(){
    int max_id = MAX_ID;
	
	NoximPacket p;
    p.src_id = local_id;
    double rnd = rand() / (double) RAND_MAX;
    double range_start = 0.0;
	int re_transmit = 1;

    do {
		p.dst_id = randInt(0, max_id);
		assert( p.dst_id < MAX_ID + 1 );

		// TODO: know why
		// hotspot <node_id, percentage>
		for (unsigned int i = 0; i < NoximGlobalParams::hotspots.size(); i++) {
			if (rnd >= range_start && rnd <	range_start + NoximGlobalParams::hotspots[i].second) {
				if (local_id != NoximGlobalParams::hotspots[i].first) {
					p.dst_id = NoximGlobalParams::hotspots[i].first;
				}	
				break;
			} 
			else range_start += NoximGlobalParams::hotspots[i].second;
		}

		if (p.dst_id == p.src_id)
			re_transmit = 1;
		else{
			re_transmit = !TLA(p);
			TAAR(p);
		}
    } while ((p.dst_id == p.src_id) || re_transmit);
	
	assert( NoximGlobalParams::message_level || p.routing >= 0 );
	p.timestamp = getCurrentCycleNum();
	p.size = p.flit_left = randInt(NoximGlobalParams::min_packet_size, NoximGlobalParams::max_packet_size);
    return p;
}

int NoximProcessingElement_en::randInt(int min, int max){
    return min + (int) ((double) (max - min + 1) * rand() / (RAND_MAX + 1.0));
}

NoximPacket NoximProcessingElement_en::trafficTranspose1(){
    NoximPacket p;
    p.src_id = local_id;
    NoximCoord src, dst;

    src   = id2Coord(p.src_id);
	dst.x = NoximGlobalParams::mesh_dim_x - 1 - src.y;
    dst.y = NoximGlobalParams::mesh_dim_y - 1 - src.x;
    dst.z = NoximGlobalParams::mesh_dim_z - 1 - src.z;
	fixRanges(src, dst);
    p.dst_id = coord2Id(dst);

    p.timestamp = getCurrentCycleNum() ;
    p.size = p.flit_left = randInt(NoximGlobalParams::min_packet_size, NoximGlobalParams::max_packet_size);
	
	/*** Cross Layer Solution***/
	bool tmp = !TLA(p);
	if(tmp && !NoximGlobalParams::message_level)
		p.dst_id = NOT_VALID;
	
	TAAR(p); // TODO: tla + taar

    return p;
}

void NoximProcessingElement_en::fixRanges(const NoximCoord src, NoximCoord & dst){
    // Fix ranges
    if (dst.x < 0)
		dst.x = 0;

    if (dst.y < 0)
		dst.y = 0;

	if (dst.z < 0)
		dst.z = 0;

    if (dst.x >= NoximGlobalParams::mesh_dim_x)
		dst.x = NoximGlobalParams::mesh_dim_x - 1;

    if (dst.y >= NoximGlobalParams::mesh_dim_y)
		dst.y = NoximGlobalParams::mesh_dim_y - 1;

	if (dst.z >= NoximGlobalParams::mesh_dim_z)
		dst.z = NoximGlobalParams::mesh_dim_z - 1;
}

