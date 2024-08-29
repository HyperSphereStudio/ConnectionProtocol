
#include <stdint.h>
#include <cstring>

#ifdef EXPORT_WINDOWS
    #define EXPORT extern "C" __declspec(dllexport)
    #warning Exporting to windows!
#else
    #define EXPORT
#endif

/*
    SyncByte::UInt16
    SIZE::UInt16 (big endian)
    PacketHash::UInt8 -> u8hash (defined below) (skip over packethash for v0)
    PAYLOAD::SIZE
*/
namespace Simple{
      
struct ConnectionProtocol{
    typedef void on_packet_event_f(uint8_t* packet, int n, void* f_data);
    
private:
    uint8_t *write_buffer;
    uint16_t read_buffer_capacity, read_buffer_size;

    enum State{
        WaitForSync1,
        WaitForSync2,
        WaitForSize16,
        WaitForSize8,
        WaitForHash,
        WaitForPayload,
        Start = WaitForSync1
    };

    State state;
    uint16_t packetSize, bytesRemaining;
    uint16_t packetHash;
    uint8_t* packetData;

    static uint8_t packethash(uint8_t* payload, uint16_t payload_size){
        uint8_t sizearray[2] = {(uint8_t) (payload_size >> 8), (uint8_t) payload_size};
        auto headerhash = u8hash(sizearray, 2);
        auto bodyhash = u8hash(payload, payload_size);
        return headerhash ^ bodyhash;
    }

    static uint8_t u8hash(uint8_t* buffer, int size){
        uint16_t v0 = 0xFE;
        for(int i = 0; i < size; i++){
            uint8_t v1 = buffer[i];
            uint8_t t1 = (v1 ^ 0x3713) << 2, t0 = (v0 ^ 0x9171) >> 2;
            v0 = v0 ^ t0 ^ v1 ^ t1 ^ (v1&t0) ^ (v0&t1); 
        }
        return v0;
    }

    inline bool parseByte(uint8_t* value, uint8_t** data, uint16_t* len){
        if(*len > 0){
            *len = *len - 1;
            *value = **data;
            *data = *data + 1;
            return true;
        }
        return false;
    }

    void parsePacket(uint8_t** data, uint16_t* dataLen, on_packet_event_f* on_packet, void* on_packet_arg){
        uint8_t v;

        if(state == WaitForSync1){
            while(parseByte(&v, data, dataLen)){
                if(v == Sync1){
                    state = WaitForSync2;
                    break;
                }
            }
            if(state == WaitForSync1) return;
        }

        if(state == WaitForSync2){ 
            if(parseByte(&v, data, dataLen))
                state = v == Sync2 ? WaitForSize16 : Start;
            else return;
        }

        if(state == WaitForSize16){
            if(parseByte(&v, data, dataLen)){
                packetSize = v << 8;
                state = WaitForSize8;
            } else return;
        } 

        if(state == WaitForSize8){
            if(parseByte(&v, data, dataLen)){
                packetSize += v;
                bytesRemaining = packetSize;
                if(packetSize > read_buffer_capacity){      //Should be max packet size
                    state = Start;
                    error_bytes_read += PacketHashOffset;
                    return;
                }else state = WaitForHash;
            } else return;
        } 

        if(state == WaitForHash){
             if(parseByte(&v, data, dataLen)){
                packetHash = v;
                state = WaitForPayload;
            } else return;
        }

        if(state == WaitForPayload){
            uint16_t n = bytesRemaining > *dataLen ? *dataLen : bytesRemaining;
            bytesRemaining -= n;
            uint8_t* payload_buffer = packetData;
            if(*dataLen >= packetSize)  //No need to copy to internal buffer if the whole packet is in this buffer
                payload_buffer = *data;
            else {  //Force copy to internal buffer
                memcpy(packetData + read_buffer_size, *data, n);
                read_buffer_size += n;
            }      
            *dataLen -= n;               
            if(bytesRemaining == 0){
                state = Start;
                read_buffer_size = 0;
                if(packethash(payload_buffer, packetSize) == packetHash){  
                    on_packet(payload_buffer, packetSize, on_packet_arg);
                }else
                    error_bytes_read += PayloadOffset + packetSize;
            }
        }
    }

public:
    static const uint8_t Sync1 = 0xDE, Sync2 = 0xAD;
    static const int Sync1Offset = 0, Sync2Offset = 1, Size16Offset = 2, Size8Offset = 3, PacketHashOffset = 4, PayloadOffset = 5;
    static const int MaxPayloadSize = 0xFFFF - PayloadOffset, MinPacketSize = PayloadOffset;

    uint32_t error_bytes_read = 0;

    ConnectionProtocol(uint8_t* rx_buffer, uint8_t* tx_buffer, uint16_t read_buffer_capacity) : 
        packetData(rx_buffer), 
        write_buffer(tx_buffer),
        read_buffer_capacity(read_buffer_capacity),
        read_buffer_size(0),
        state(Start){}    

    uint16_t recieve(uint8_t* data, uint16_t dataLen, on_packet_event_f* on_packet, void* on_packet_arg){
        uint16_t bytes_read = 0;
        while(dataLen > 0){
            uint8_t* temp_data = data + bytes_read;
            uint16_t oldLen = dataLen;
            parsePacket(&temp_data, &dataLen, on_packet, on_packet_arg);
            bytes_read += oldLen - dataLen;
        }
        return bytes_read;
    }

    static uint16_t minBufferSize(uint16_t maxPayloadSize){ return maxPayloadSize + PayloadOffset; }
    uint8_t* packet(){ return write_buffer; }
    uint8_t* payload(){ return write_buffer + PayloadOffset; }

    uint16_t encodePacket(uint16_t size){
       write_buffer[Sync1Offset] = Sync1;
       write_buffer[Sync2Offset] = Sync2;
       write_buffer[Size16Offset] = size >> 8;
       write_buffer[Size8Offset] = size;
       write_buffer[PacketHashOffset] = packethash(payload(), size);
       return size + PayloadOffset;
    }
};

};

EXPORT Simple::ConnectionProtocol* SimpleConnectionProtocol_new(
    uint8_t* rx_buffer, uint8_t* tx_buffer, uint16_t read_buffer_capacity){
        return new Simple::ConnectionProtocol(rx_buffer, tx_buffer, read_buffer_capacity);
}
EXPORT void SimpleConnectionProtocol_free(Simple::ConnectionProtocol* p){ delete p; }
EXPORT uint16_t SimpleConnectionProtocol_payloadOffset(){ return Simple::ConnectionProtocol::PayloadOffset; }
EXPORT uint16_t SimpleConnectionProtocol_encodePacket(Simple::ConnectionProtocol* p, uint16_t packet_size){ return p->encodePacket(packet_size); }
EXPORT uint16_t SimpleConnectionProtocol_recieve(Simple::ConnectionProtocol* p, uint8_t* data, uint16_t dataLen, Simple::ConnectionProtocol::on_packet_event_f* on_packet, void* on_packet_arg){ return p->recieve(data, dataLen, on_packet, on_packet_arg); }
EXPORT uint16_t SimpleConnectionProtocol_recieveChar(Simple::ConnectionProtocol* p, uint8_t data, Simple::ConnectionProtocol::on_packet_event_f* on_packet, void* on_packet_arg){ return SimpleConnectionProtocol_recieve(p, &data, 1, on_packet, on_packet_arg); }
EXPORT uint32_t SimpleConnectionProtocol_errorCount(Simple::ConnectionProtocol* p){ return p->error_bytes_read; }
EXPORT uint16_t SimpleConnectionProtocol_minBufferSize(uint16_t maxPayloadSize){ return Simple::ConnectionProtocol::minBufferSize(maxPayloadSize); }

/*
#include <iostream>
#include <cassert>
using namespace std;

int packets = 0;
void on_packet(uint8_t* v, int n, void* data){
  for(int i = 0; i < 11; i++)
    if(v[i] != i) return;
  packets++;
}

int main()
{
    uint8_t rx_buffer[128], tx_buffer[128];
    auto prot = SimpleConnectionProtocol_new(rx_buffer, tx_buffer, 128);

    auto write_packet = &tx_buffer[SimpleConnectionProtocol_payloadOffset()];
    for(int i = 0; i < 11; i++)
        write_packet[i] = i;
    auto packetLength = SimpleConnectionProtocol_encodePacket(prot, 11);
    auto packet = tx_buffer;

    cout << "PacketSize:" << packetLength << endl;

    for(int i = 0; i < packetLength; i++)
        packet[i + packetLength] = packet[i];       //Create double packet on same buffer

    auto lpack = 0;

    cout << "Rx1:" << SimpleConnectionProtocol_recieveChar(prot, 'g', &on_packet, nullptr) << endl;
    cout << "Rx16:" << SimpleConnectionProtocol_recieve(prot, packet, packetLength, &on_packet, nullptr) << endl;   //Best Case
    assert(lpack + 1 == packets);
    lpack = packets;

    for(int i = 0; i < packetLength; i++)
        SimpleConnectionProtocol_recieve(prot, &packet[i], 1, &on_packet, nullptr);      //Byte by byte
    assert(lpack + 1 == packets);
    lpack = packets;

    uint8_t randomData[50000];
    for(int i = 0; i < 50000; i++){
        randomData[i] = (uint8_t) std::rand();
    }
    SimpleConnectionProtocol_recieve(prot, randomData, 50000, &on_packet, nullptr);      //Massive random data               //Random Data
    SimpleConnectionProtocol_recieve(prot, packet, packetLength, &on_packet, nullptr);   //Best Case
    assert(lpack + 1 == packets);
    lpack = packets;

    SimpleConnectionProtocol_recieve(prot, packet, packetLength*2, &on_packet, nullptr);   //Double packet
    assert(lpack + 2 == packets);
    lpack = packets;

    //Packet Way to Big for Buffer (should do nothing)
    SimpleConnectionProtocol_encodePacket(prot, 15000);
    SimpleConnectionProtocol_recieve(prot, packet, packetLength, &on_packet, nullptr);
    assert(lpack == packets);
    lpack = packets;
    SimpleConnectionProtocol_encodePacket(prot, 11);

    SimpleConnectionProtocol_recieve(prot, packet, packetLength, &on_packet, nullptr);
    assert(lpack + 1 == packets);
    lpack = packets;

    uint8_t malformedHash[] = {0xDE, 0xAD, 0, 2, 13, 0, 0};   //Incorrect hash
    SimpleConnectionProtocol_recieve(prot, malformedHash, 7, &on_packet, nullptr);
    assert(lpack == packets);
    lpack = packets;

    uint8_t malformedSync2[] = {0xDE, 3};
    SimpleConnectionProtocol_recieve(prot, malformedSync2, 2, &on_packet, nullptr);
    assert(lpack == packets);
    lpack = packets;

    SimpleConnectionProtocol_recieve(prot, packet, packetLength, &on_packet, nullptr);
    assert(lpack + 1 == packets);
    lpack = packets;

    SimpleConnectionProtocol_free(prot);
    cout << "Free" << endl;

    return 0;
}
*/