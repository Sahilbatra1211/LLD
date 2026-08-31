#pragma once

#include "Modals/Floor.h"
#include "Modals/ParkingSlot.h"
#include "Modals/Ticket.h"
#include "Modals/Vehicle.h"
#include "Strategy/PaymentStrategy.h"
#include "Strategy/SlotAvailableStrategy.h"
#include <bits/stdc++.h>
#include <stdexcept>

using namespace std;

class ParkingLot {
    vector<Floor*> floors;
    SlotAvailableStrategy *slotAvailableStrategy;
    vector<Ticket*> tickets;
    PaymentStrategy *paymentStrategy;

public:
    ParkingLot(vector<Floor*> floors,SlotAvailableStrategy* slotAvailableStrategy, PaymentStrategy *paymentStrategy):floors(floors),
    slotAvailableStrategy(slotAvailableStrategy),
    paymentStrategy(paymentStrategy){

    }

    bool entry(Vehicle *vehicle){
        // first is check availability
        try{
            ParkingSlot* parkingSlot=slotAvailableStrategy->findSlot(floors);

            // now we need to book that parking slot
            parkingSlot->setOccupied(true);
            
            // now we need to generate the ticket
            Ticket* ticket= generateEntryTicket(vehicle, parkingSlot, 1);
            tickets.push_back(ticket);

            //ticket generated slot booked now we can return true
            return true;

        }
        catch(runtime_error e){
            cout<<"No slots available";
            return false;
        }
    }

    Ticket* generateEntryTicket(Vehicle *vehicle,ParkingSlot *parkingSlot, int entryTime){
        return new Ticket(vehicle,parkingSlot,entryTime);
    }

    bool exit(Vehicle *vehicle){
        // first take payment
        int payment=paymentStrategy->calculate(1, 2);

        // get ticket and update it
        Ticket *ticket=findTicket(vehicle);

        // update the ticket 
        // not implemented yet.

        return true;

    }

    Ticket *findTicket(Vehicle *vehicle){
        for(auto node: tickets){
            if(node->vehicle==vehicle){
                return node;
            }
        }
        throw runtime_error("ticket not found");
    }
};
