#include <math.h>
#include "NoximRouter_en.h"
#include "NoximStats.h"

extern int wait_cnt[200];

void NoximRouter_en::rxProcess(){
	int i,j,k;
	if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) {
		cout<<"Router[" << local_id << "]-Rx"<<endl;
	}

    if (reset.read()) {
		for ( i = 0; i < DIRECTIONS + 2; i++){ 
			ack_rx[i].write(1);
			routed_flits[i] = 0;
			buffer[i].Clean();
		}
		reservation_table.clear();
		routed_packets = 0;
    } 

	else {
		for ( i = 0; i < DIRECTIONS + 2; i++) {
			if ( (req_rx[i].read()==1) && ack_rx[i].read()==1 ){
				if ( !_emergency ){
					NoximFlit received_flit = flit_rx[i].read();
					if (NoximGlobalParams::verbose_mode > VERBOSE_OFF  ) {
						cout << getCurrentCycleNum() << ": Router[" << local_id << "], Input[" << i
						<< "], Received flit: " << received_flit << endl;
					}
					received_flit.waiting_cnt = getCurrentCycleNum();
					buffer[i].Push(received_flit);
					routed_flits[i]++;
					if( received_flit.flit_type == FLIT_TYPE_HEAD ) routed_packets++;
					if ( i != DIRECTION_UP && i != DIRECTION_DOWN)
						stats.power.RouterRxLateral();
				}
			}
			ack_rx[i].write((!buffer[i].IsFull()) && (!_emergency));
		}
    }
	
	if ( !_emergency )
		stats.power.TileLeakage();

	if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) 
		cout<<"Router[" << local_id << "]-Rx ends"<<endl;
}

void NoximRouter_en::txProcess(){
	if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) {
		cout<<"Router[" << local_id << "]-Tx"<<endl;
    }

    if (reset.read()) {
		for (int i = 0; i < DIRECTIONS + 2; i++){
			req_tx[i].write(0);
			waiting[i]     = 0;
		}
		_total_waiting  = 0;
		for (int i = 0; i < 200; i++){ 
			wait_cnt[i]     = 0;
		}
    }
	else {
		// 1st phase: Reservation
		for (int j = 0; j < DIRECTIONS + 2; j++) {
			int i = (start_from_port + j) % (DIRECTIONS + 2);
			int o;
			if (!buffer[i].IsEmpty()) {
				NoximFlit flit = buffer[i].Front();	
				if (flit.flit_type == FLIT_TYPE_HEAD) {
					NoximRouteData route_data;
					route_data.current_id = local_id;
					route_data.src_id     = (flit.arr_mid)?flit.mid_id:flit.src_id;
					route_data.dst_id     = (flit.arr_mid)?flit.dst_id:flit.mid_id;
					route_data.dir_in     = i;
					route_data.routing    = flit.routing_f;
					route_data.DW_layer   = flit.DW_layer;
					route_data.arr_mid    = flit.arr_mid;
					if( flit.routing_f < 0 ){
						cout<<getCurrentCycleNum()<<":"<<flit<<endl;
						cout<<"flit.current_id"<<"="<<local_id       <<id2Coord(route_data.current_id)<<endl; 
						cout<<"flit.src_id    "<<"="<<flit.src_id    <<id2Coord(flit.src_id          )<<endl;
						cout<<"flit.mid_id    "<<"="<<flit.mid_id    <<id2Coord(flit.mid_id          )<<endl; 		
						cout<<"flit.dst_id    "<<"="<<flit.dst_id    <<id2Coord(flit.dst_id          )<<endl; 
						cout<<"flit.dir_in    "<<"="<<i              <<endl; 
						cout<<"flit.routing   "<<"="<<flit.routing_f <<endl; 
						cout<<"flit.DW_layer  "<<"="<<flit.DW_layer  <<endl; 
						cout<<"flit.arr_mid   "<<"="<<flit.arr_mid   <<endl; 
						assert(false);
					}
					if (NoximGlobalParams::verbose_mode > VERBOSE_LOW ) {
						cout<<"Before route:"<<flit;
                    }
					o = route(route_data);
                    if (reservation_table.isAvailable(o) && (getCurrentCycleNum()%DFS)<(8-RST))
					{     
						reservation_table.reserve(i, o);
						if (NoximGlobalParams::verbose_mode > VERBOSE_OFF ) {
							cout << getCurrentCycleNum()
							<< ": Router[" << local_id
							<< "], Input[" << i << "] (" << buffer[i].
							Size() << " flits)" << ", reserved Output["
							<< o << "], flit: " << flit << endl;
						}
						stats.power.ArbiterNControl();	
					}
				}
			}
		}
		start_from_port++;
	
		// 2nd phase: Forwarding
		for(int o=0; o<DIRECTIONS+2; o++)
			if ( 1 == ack_tx[o].read() ) req_tx[o].write(0);
			
		for (int i = 0; i < DIRECTIONS + 2; i++) {
			if (!buffer[i].IsEmpty() && !_emergency ) {
				NoximFlit flit = buffer[i].Front();
				int o = reservation_table.getOutputPort(i);	
				if (o != NOT_RESERVED) {
					if ( 1 == ack_tx[o].read()) {
						if (NoximGlobalParams::verbose_mode > VERBOSE_OFF ) {
							cout << getCurrentCycleNum()
							<< ": Router[" << local_id
							<< "], Input[" << i <<
							"] forward to Output[" << o << "], flit: "
							<< flit << endl;
						}
					if((getCurrentCycleNum()%DFS)<(8-RST)){
						flit_tx[o].write(flit);
						req_tx [o].write(!buffer[i].IsEmpty());
						buffer [i].Pop();
						waiting[i] = 0 ;
					
						if( o != DIRECTION_UP && o != DIRECTION_DOWN )
							stats.power.Router2Lateral();			
						
						if (flit.flit_type == FLIT_TYPE_TAIL)
							reservation_table.release(o);
											
						_total_waiting += getCurrentCycleNum() - flit.waiting_cnt;

						if (flit.flit_type == FLIT_TYPE_HEAD)
							if(((getCurrentCycleNum() - flit.waiting_cnt) < 200) && (getCurrentCycleNum() > 400000))
								wait_cnt[(getCurrentCycleNum() - flit.waiting_cnt)]++;						
			
						if (o == DIRECTION_LOCAL) {
							stats.receivedFlit(getCurrentCycleNum(), flit);
							stats.power.Router2Local();
							// if (NoximGlobalParams::max_volume_to_be_drained) {
							// 	if (drained_volume >= NoximGlobalParams::max_volume_to_be_drained)
							// 		sc_stop();
							// 	else {
							// 		drained_volume++;
							// 		local_drained++;
							// 	}
							// }
						}
					 }
					}
					else{
						waiting[i]++;
					}
				}
				else{
					waiting[i]++;
				}
			}
		}
    }
}

int NoximRouter_en::route(const NoximRouteData & route_data)
{
    if (route_data.dst_id == local_id && route_data.arr_mid)
		return DIRECTION_LOCAL;
	
	else if ( (route_data.dst_id == local_id && !route_data.arr_mid))
		return DIRECTION_SEMI_LOCAL;
	
	vector < int >candidate_channels = routingFunction(route_data);

	// for(int i = candidate_channels.size() - 1  ; i >= 0 ; i--){
	// 	if ( candidate_channels[i] < 4 ) {//for lateral direction
	// 		if ( on_off_neighbor[ candidate_channels[i] ].read() == 1 ){//if the direction is throttled
	// 			candidate_channels.erase( candidate_channels.begin() + i);	
	// 		}
	// 	}
	// }

    NoximCoord position = id2Coord(local_id); 

	if( candidate_channels.size() == 0){
		// if(!on_off_neighbor[DIRECTION_DOWN]  && position.z < 3)//if the direction is not throttled
			candidate_channels.push_back(DIRECTION_WEST);
	}

    return selectionFunction(candidate_channels, route_data);
}

vector < int >NoximRouter_en::routingFunction(const NoximRouteData & route_data)
{
    NoximCoord position  = id2Coord(route_data.current_id);
    NoximCoord src_coord = id2Coord(route_data.src_id    );
    NoximCoord dst_coord = id2Coord(route_data.dst_id    );
    int dir_in           = route_data.dir_in  ;
	int routing          = route_data.routing ;  
	int DW_layer         = route_data.DW_layer;
	int arr_mid          = route_data.arr_mid ;
	
    switch ( NoximGlobalParams::routing_algorithm ) {
	case ROUTING_YLY:
	  return routingWestFirst_yly(position, dst_coord);
    default:
		assert(false);
    }

    return (vector <int>) (0);
}

int NoximRouter_en::selectionFunction(const vector < int >&directions,
				   const NoximRouteData & route_data)
{
    if (directions.size() == 1)
	return directions[0];

    switch (NoximGlobalParams::selection_strategy) {
    case SEL_RANDOM      :return selectionRandom     (directions);
    // case SEL_BUFFER_LEVEL:return selectionBufferLevel(directions);
    // case SEL_NOP         :return selectionNoP        (directions, route_data);
	// case SEL_RCA         :return selectionRCA2D      (directions, route_data);
	// case SEL_PROPOSED    :return selectionProposed   (directions, route_data);
	// case SEL_THERMAL     :return selectionThermal    (directions, route_data);
	default              :assert(false);
    }
    return 0;
}

int NoximRouter_en::selectionRandom(const vector < int >&directions)
{
    return directions[rand() % directions.size()];
}

vector < int >NoximRouter_en::routingWestFirst_yly(const NoximCoord & current,
					    const NoximCoord & destination)
{
	vector<int> directions;

    const int dx = destination.x - current.x;                                                                                                                                                                                            
    const int dy = destination.y - current.y;
    const int dz = destination.z - current.z;                                                                                                                                                                                            
                  
    if (dx < 0) {
        directions.push_back(DIRECTION_WEST);
        return directions;
    }

    if (dx > 0)                                                                                                                                                                                                                          
        directions.push_back(DIRECTION_EAST);
                                                                                                                                                                                                                                           
    if (dy > 0) 
        directions.push_back(DIRECTION_SOUTH);
    else if (dy < 0)
        directions.push_back(DIRECTION_NORTH);

	if (dz > 0) {
		directions.push_back(DIRECTION_DOWN);
	} else if (dz < 0) {
		directions.push_back(DIRECTION_UP);
	}
	  
	if (directions.empty())
    	assert(false);
	  
      return directions;
}

void NoximRouter_en::bufferMonitor()
{	
	int i;
    if (reset.read()) {
		for ( i = 0; i < DIRECTIONS + 1; i++)
			free_slots[i].write(buffer[i].GetMaxBufferSize());
			
		for (int i=0; i<4; i++)	
			free_slots_PE[i].write( free_slots_neighbor[i]);
	}
	else {
	    for ( i = 0; i < DIRECTIONS + 1; i++)
			free_slots[i].write(buffer[i].getCurrentFreeSlots());
	    // NoP selection: send neighbor info to each direction 'i'
	    // NoximNoP_data current_NoP_data = getCurrentNoPData();
	    // for ( i = 0; i < DIRECTIONS; i++)
		// 	NoP_data_out[i].write(current_NoP_data);
		// vertical_free_slot_out.write(current_NoP_data);
		
		for(int i=0; i<4; i++)	
			free_slots_PE[i].write( free_slots_neighbor[i] );
		// for(int i=0; i<8; i++)
		// 	RCA_PE[i].write( RCA_data_in[i] );		
		// for(int i=0; i<4; i++)		
		// 	NoP_PE[i].write( NoP_data_in[i] );
    }
}