/* 
 * File:   Reducer.cpp
 * Author: srba
 *
 * Created on 15 February 2014, 10:50
 */

#include "Reducer.h"
#include "PetriNet.h"
#include <PetriParse/PNMLParser.h>

namespace PetriEngine{
    
Reducer::Reducer(PetriNet  *net): _removedTransitions(0), _removedPlaces(0), _ruleA(0), _ruleB(0), _ruleC(0), _ruleD(0) {
        _nplaces=net->numberOfPlaces();
        _ntransitions=net->numberOfTransitions();
        unfoldTransitions = new unfoldTransitionsType[_ntransitions];
        unfoldTransitionsInit = new unfoldTransitionsType[1];
        _inArc = new int[_nplaces*_ntransitions];
        _outArc = new int[_nplaces*_ntransitions];
        for (int p=0; p<_nplaces; p++) {
            for (int t=0; t<_ntransitions; t++) {
                _inArc[_nplaces*t+p]=net->inArc(p,t);
                _outArc[_nplaces*t+p]=net->outArc(t,p);
            }
        }
}

Reducer::~Reducer() {
        delete[] unfoldTransitions;
        delete[] unfoldTransitionsInit;
        delete[] _inArc;
        delete[] _outArc;
}
    
void Reducer::CreateInhibitorPlacesAndTransitions(PetriNet* net, PNMLParser::InhibitorArcList inhibarcs, MarkVal* placeInInhib, MarkVal* transitionInInhib){
        //Initialize
        for(size_t i = 0; i < net->numberOfPlaces(); i++) {
                                placeInInhib[i] = 0;
                        }    
    
        for(size_t i = 0; i < net->numberOfTransitions(); i++) {
                                transitionInInhib[i] = 0;
                        }
        
        //Construct the inhibitor places/arcs
        PNMLParser::InhibitorArcIter arcIter;
	for(arcIter = inhibarcs.begin(); arcIter != inhibarcs.end(); arcIter++){
		size_t place = -1;
                //Find place number
		for(size_t i = 0; i < net->numberOfPlaces(); i++){
			if(net->placeNames()[i] == arcIter->source){
                                place=i;
                                placeInInhib[place] = arcIter->weight;
				break;
			}
		}
                
                size_t transition = -1;
                //Find place number
		for(size_t i = 0; i < net->numberOfTransitions(); i++){
			if(net->transitionNames()[i] == arcIter->target){
                                transition=i;
                                transitionInInhib[transition] = arcIter->weight;
				break;
			}
		}
                
		assert(place >= 0 && transition>=0);
	}
        
    }
    
    
void Reducer::Print(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib, MarkVal* transitionInInhib){
        fprintf(stdout,"\nNET INFO:\n");
        fprintf(stdout,"Number of places: %i\n",net->numberOfPlaces());
        fprintf(stdout,"Number of transitions: %i\n\n",net->numberOfTransitions());
        for (int j=0; j < net->numberOfTransitions(); j++) {
            fprintf(stdout,"Transition %d:\n",j); 
            for (int i=0; i < net->numberOfPlaces(); i++) {             
                     if (net->inArc(i,j)>0) fprintf(stdout,"   Input place %d with arc-weight %d\n",i,net->inArc(i,j));                       
            }
            for (int i=0; i < net->numberOfPlaces(); i++) {             
                     if (net->outArc(j,i)>0) fprintf(stdout,"  Output place %d with arc-weight %d\n",i,net->outArc(j,i));                       
            }
                fprintf(stdout,"\n",j);   
        }
        for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Marking at place %d is: %d\n",i,m0[i]);
        } 
        for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Query count for place %d is: %d\n",i,placeInQuery[i]);
        } 
        for (int i=0; i < net->numberOfPlaces(); i++) {
                fprintf(stdout,"Inhibitor count for place %d is: %d\n",i,placeInInhib[i]);
        } 
        for (int i=0; i < net->numberOfTransitions(); i++) {
                fprintf(stdout,"Inhibitor count for transition %d is: %d\n",i,transitionInInhib[i]);
        } 
    }
    
    
 void Reducer::Reduce(PetriNet* net, MarkVal* m0, MarkVal* placeInQuery, MarkVal* placeInInhib, MarkVal* transitionInInhib, int enablereduction) {
          
     bool continueReductions=true;
     
     while (continueReductions){
         continueReductions=false; // repeat all reductions rules as long as something was reduced
         
        // fprintf(stderr,"Rule A\n");
         // Rule A  - find transition t that has exactly one place in pre and post and remove one of the places   
         for (size_t t=0; t < net->numberOfTransitions(); t++) {
                if (transitionInInhib[t]>0) { continue;} // if t has a connected inhibitor arc, it cannot be removed
                int pPre=-1;
                int pPost=-1;
                bool ok=true;
                for (size_t p=0; p < net->numberOfPlaces(); p++) {
                    if (net->inArc(p,t)>0) { 
                        if (pPre>=0) {ok=false; break;}
                        pPre=p;
                    }
                    if (net->outArc(t,p)>0) {
                        if (pPost>=0) {ok=false; break;}
                        pPost=p;
                    }
                }
                if (ok && pPre>=0 && pPost>=0) {
                    //necessary checks if pPre!=pPost
                    if (pPre!=pPost && ((m0[pPre]>0 && m0[pPost]>0) || net->inArc(pPre,t)>1 || net->outArc(t,pPost)>1 || 
                        placeInQuery[pPre]>0 || placeInQuery[pPost]>0 || placeInInhib[pPre]>0 || placeInInhib[pPost]>0 )) {
                        ok=false;
                    } 
                    if (ok && pPre!=pPost) { // check that pPre is not connected to any other transition than t
                        for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                                if (net->inArc(pPre,_t)>0 && _t != t) {ok=false; break; }
                                // if (net->outArc(_t,pPost)>0 && _t != t ) {ok=false; break; }
                        }
                    }
                    if (pPre==pPost && net->inArc(pPre,t)!=net->outArc(t,pPost)) {ok=false;} //self-loops should have same but arbitrary weight
                    if (ok) {
                      continueReductions=true;
                      _ruleA++;
                      if (pPre!=pPost) { // self-loops can be removed without changing anything else
                        // Remember that if the initial marking has tokens in pPre, we should fire  t initially
                            if (m0[pPre]>0) {
                                std::pair<int, int> element(t, m0[pPre]);
                                unfoldTransitionsInit->push_back(element);
                            } 
                        // Remember that after any transition putting to pPre, we should fire immediately after that also t
                             for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                                if (net->outArc(_t,pPre)>0) {
                                   std::pair<int, int> element(t, net->outArc(_t,pPre));
                                   unfoldTransitions[_t].push_back(element);
                                }
                             }    
                        // Remove transition t and the place that has no tokens in m0
            //            fprintf(stderr,"Removing transition %i connected pre-place %i and post-place %i\n",(int)t,(int)pPre,(int)pPost);
                        net->updateinArc(pPre,t,0);
                        net->updateoutArc(t,pPost,0);
                        _removedTransitions++;
                        if (m0[pPre]==0) { // removing pPre
            //                    fprintf(stderr,"Removing place %i\n",(int)pPre);
                                _removedPlaces++;
                                for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                                    net->updateoutArc(_t,pPost,net->outArc(_t,pPost)+net->outArc(_t,pPre));
                                    net->updateoutArc(_t,pPre,0);
                                }    
                        } else if (m0[pPost]==0) { // removing pPost
           //             fprintf(stderr,"Removing place %i\n",(int)pPost);
                        _removedPlaces++;
                            for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                                net->updateinArc(pPre,_t,net->inArc(pPost,_t));
                                net->updateoutArc(_t,pPre,net->outArc(_t,pPre)+net->outArc(_t,pPost));
                                net->updateinArc(pPost,_t,0);
                            }
                        }
                      } else { // pPre==pPost
                                net->updateinArc(pPre,t,0);
                                net->updateoutArc(t,pPost,0);
                                _removedTransitions++;  
                      }
                    }
                }              
         } // end of Rule A main for-loop
         
      
    if (enablereduction==1) {     // only allowed in aggresive reductions (it changes k-boundedness)
       //  fprintf(stderr,"Rule B\n");
         // Rule B - find place p that has exactly one transition in pre and exactly one in post and remove the place
         for (size_t p=0; p < net->numberOfPlaces(); p++) {
             if (placeInInhib[p]>0 || m0[p]>0) { continue;} // if p has inhibitor arc or nonzero initial marking it cannot be removed
             int tPre=-1;
             int tPost=-1;
             bool ok=true;
             for (size_t t=0; t < net->numberOfTransitions(); t++) {
                 if (net->outArc(t,p)>0) {
                     if (tPre>=0) {tPre=-1; ok=false; break;}
                     tPre=t;
                 }
                 if (net->inArc(p,t)>0) {
                     if (tPost>=0) {tPost=-1; ok=false; break;}
                     tPost=t;
                 }
             }
             if (ok && tPre>=0 && tPost>=0 && tPre!=tPost && (net->outArc(tPre,p)==net->inArc(p,tPost)) && 
                 placeInQuery[p]==0 && placeInInhib[p]==0 && m0[p]==0 && transitionInInhib[tPre]==0 && transitionInInhib[tPost]==0) {
                 // Check if the output places of tPost do not have any inhibitor arcs connected
                 for (size_t _p=0; _p < net->numberOfPlaces(); _p++) {
                     if (net->outArc(tPost,_p)>0 && placeInInhib[_p]>0) {ok=false; break;}
                 }
                 // Check that there is no other place than p that gives to tPost, tPre can give to other places
                 for (size_t _p=0; _p < net->numberOfPlaces(); _p++) {
                    // if (net->outArc(tPre,_p)>0 && _p != p) {ok=false; break; }
                     if (net->inArc(_p,tPost)>0 && _p != p ) {ok=false; break; }
                 }
                 if (ok) {
                     continueReductions=true;
                     _ruleB++;
                     // Remember that after tPre we should always fire also tPost
                     std::pair<int, int> element(tPost,1);
                     unfoldTransitions[tPre].push_back(element);
                     // Remove place p
         //            fprintf(stderr,"Removing place %i connected pre-transition %i and post-transition %i\n",(int)p,(int)tPre,(int)tPost);
                     net->updateoutArc(tPre,p,0);
                     net->updateinArc(p,tPost,0);
                     _removedPlaces++;
         //            fprintf(stderr,"Removing transition %i\n",(int)tPost);
                     _removedTransitions++;
                     for (size_t _p=0; _p < net->numberOfPlaces(); _p++) { // remove tPost
                         net->updateoutArc(tPre,_p,net->outArc(tPre,_p)+net->outArc(tPost,_p));
                         net->updateoutArc(tPost,_p,0);
                     }
                 }
             }
         } // end of Rule B main for-loop
    }     
        
    if (enablereduction==1) {     // only allowed in aggresive reductions (it changes k-boundedness)
      //   fprintf(stderr,"Rule C\n");
         // Rule C - two transitions that put and take from the same places
         for (size_t t1=0; t1 < net->numberOfTransitions(); t1++) {
                for (size_t t2=0; t2 < net->numberOfTransitions(); t2++) {
                    if (t1!=t2) { 
                        // check what places have as pre t1 and as post t2
                        int noOfPlaces=0; // how many such places we have
                        bool removePlace[net->numberOfPlaces()]; // remember what places can be removed
                        for (size_t p=0; p < net->numberOfPlaces(); p++) {removePlace[p]=false;}
                        
                        for (size_t p=0; p < net->numberOfPlaces(); p++) {
                                if (net->outArc(t1,p)==net->inArc(p,t2) && net->outArc(t1,p)>0) {                                    
                                        // check that the places in between are not connected to any other transitions                                
                                        bool ok=true;
                                        for (size_t _t=0; _t < net->numberOfTransitions(); _t++) {
                                                if (net->outArc(_t,p)>0 && _t != t1) {ok=false; break;}
                                                if (net->inArc(p,_t)>0 && _t != t2) {ok=false; break;}                                        
                                        }
                                        if (ok) { removePlace[p]=true; noOfPlaces++; }
                                }                             
                        }                    
                        if (noOfPlaces>=2) { 
          // Remove places that are in post of t1, are not in queries, are not inhibitor places and have empty initial marking, one place must be left
                           for (size_t p=0; p < net->numberOfPlaces(); p++) {
                               if (removePlace[p] && noOfPlaces>=2 && placeInQuery[p]==0 && placeInInhib[p]==0 && m0[p]==0) {
                                   continueReductions=true;
                                   _ruleC++;
      //                             fprintf(stderr,"Removing place %i\n",(int)p);
                                   net->updateoutArc(t1,p,0);
                                   net->updateinArc(p,t2,0);
                                   _removedPlaces++;
                                   noOfPlaces--;
                               }
                           }
                        }
                    }                        
                }
         }
    }    
         
     //    fprintf(stderr,"Rule D\n");
         // Rule D - two transitions with the same pre and post and same inhibitor arcs 
         for (size_t t1=0; t1 < net->numberOfTransitions(); t1++) {
                for (size_t t2=0; t2 < net->numberOfTransitions(); t2++) {
                    if (t1!=t2 && transitionInInhib[t1]==transitionInInhib[t2]) {                       
                        bool ok=false;
                        for (size_t p=0; p < net->numberOfPlaces(); p++) {
                            if (net->inArc(p,t1)!=net->inArc(p,t2) || net->outArc(t1,p)!=net->outArc(t2,p)) {ok=false; break;}
                            if (net->inArc(p,t2)>0 || net->outArc(t2,p)>0) {ok=true;} //so that we do not remove isolated transitions 
                        }
                        if (ok) { // Remove transition t2
                            continueReductions=true;
                            _ruleD++;
                            _removedPlaces++;                           
     //                       fprintf(stderr,"Removing transition %i\n",(int)t2);                                                             
                            for (size_t p=0; p < net->numberOfPlaces(); p++) {                                                                                           
                                   net->updateoutArc(t2,p,0);
                                   net->updateinArc(p,t2,0);
                                   
                            }
                        }
                     }
                }                        
         }
         
     } // end of main while-loop   
}
    
void Reducer::expandTrace (unsigned int t, std::vector<unsigned int>& trace) {
         trace.push_back(t);
         for(unfoldTransitionsType::iterator it = unfoldTransitions[t].begin(); it != unfoldTransitions[t].end(); it++) {
             for (int i=1; i<= it->second; i++) {
                 expandTrace(it->first, trace);
             }        
         }
 }
 
const std::vector<unsigned int> Reducer::NonreducedTrace(PetriNet* net, const std::vector<unsigned int>& trace) {
    // recover the original net
    for (int p=0; p<_nplaces; p++) {
            for (int t=0; t<_ntransitions; t++) {
                net->updateinArc(p,t,_inArc[_nplaces*t+p]);
                net->updateoutArc(t,p,_outArc[_nplaces*t+p]);
            }
        }
    // compute the expanded (nonreduced) trace)
    std::vector<unsigned int> nonreducedTrace;
    
    // first expand the transitions that can be fired from the initial marking
    for(unfoldTransitionsType::iterator it = unfoldTransitionsInit->begin(); it != unfoldTransitionsInit->end(); it++) {
        for (int i=1; i<= it->second; i++) {
     //       expandTrace(it->first,nonreducedTrace);
            nonreducedTrace.push_back(it->first);
        }
    }
    // now expand the transitions from the trace
    for(size_t i = 0; i < trace.size(); i++){
        expandTrace(trace[i],nonreducedTrace);
    }
    
    return nonreducedTrace; // this makes a copy of the vector, but the slowdown is insignificant
}


} //PetriNet namespace
