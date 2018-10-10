#ifndef _RINGNETWORK_H_
#define _RINGNETWORK_H_

#include "mbed.h"
#include "..\os\CoreModule.h"

class RingNetworkProtocol {
public:
  static const uint32_t packet_maxsize = 265;
  static const uint8_t ttl_max = 10;
  static const uint32_t device_name_maxsize = 16;
  static const uint8_t protocol_msgid_free = 0;
  static const uint8_t protocol_msgid_addressclaim = 1;
  static const uint8_t protocol_msgid_whoareyou = 2;
  static const uint8_t protocol_msgid_hello = 3;
};

struct __packed RingPacketHeader {
  uint8_t data_size;
  uint8_t control;
  uint8_t src_address;
  uint8_t dst_address;
  uint8_t ttl;
};

struct __packed RingPacketFooter {
  uint32_t hash;
};

struct __packed RingPacket {
  RingPacketHeader header;
  uint8_t data[256];
  RingPacketFooter footer;

  inline bool isProtocolPacket() {
    return (header.control & 1) == 0;
  }
  inline bool isFreePacket() {
    return isProtocolPacket() && header.data_size == 0;
  }
  inline bool isForDstAddress(uint8_t dst_address) {
    return header.dst_address == dst_address;
  }

  void setFreePacket() {
    header.control = 0;
    header.data_size = 0;
    header.src_address = 0;
    header.dst_address = 0;
    header.ttl = RingNetworkProtocol::ttl_max;
  }

  void setHelloUsingSrcAsDst(uint8_t newSrcAddress) {
    header.control = 0;
    header.data_size = 1;
    header.dst_address = header.src_address;
    header.src_address = newSrcAddress;
    header.ttl = RingNetworkProtocol::ttl_max;
    data[0] = RingNetworkProtocol::protocol_msgid_hello;
  }
};

enum PTxAction
{
  PassAlongDecreasingTTL,
  SendFreePacket,
  Send
};

class RingNetwork : public CoreModule
{
public:
  RingNetwork(PinName TxPin, PinName RxPin, uint32_t hardwareId);

  // --- CoreModule ---
  const char* getName() { return "RingNetwork"; }
  void init(const bitLabCore*);
  void mainLoop();
  void tick(millisec64 timeDelta);
  // ------------------

  void attachDataPacketReceived(Callback<void(RingPacket*, PTxAction*)> dataPacketReceived) { this->dataPacketReceived = dataPacketReceived; }
  void attachFreePacketReceived(Callback<void(RingPacket*, PTxAction*)> freePacketReceived) { this->freePacketReceived = freePacketReceived; }

  bool packetReceived() { return rx_packet_ready; }
  RingPacket* getPacket() { return &rx_packet; }
  void receiveNextPacket() { rx_packet_ready = false; }

  void sendPacket(const RingPacket* packet);

  inline bool isAddressAssigned() { return mac_state == Idle; }
  inline uint8_t getAddress() { return mac_address; }

private:
  Serial serial;
  Callback<void(RingPacket*, PTxAction*)> dataPacketReceived;
  Callback<void(RingPacket*, PTxAction*)> freePacketReceived;

  enum MacState
  {
    AddressNotAssigned,
    AddressClaiming,
    Idle
  };
  uint32_t hardware_id;
  MacState mac_state;
  uint8_t mac_address;
  char mac_device_name[RingNetworkProtocol::device_name_maxsize];

  enum MacWatcherState
  {
    Start,
    WaitingSilence,
    WaitingAfterSilence
  };
  MacWatcherState mac_watcher_state;
  volatile millisec64 mac_watcher_timeout;

  void mainLoop_UpdateWatcher(bool packetReceived);
  void mainLoop_UpdateMac(RingPacket *p);

  //data for tx
  enum TxState
  {
    TxIdle,
    SendStartByte,
    SendPacket,
    TxEscape,
    SendEndByte
  };
  volatile TxState tx_state;
  uint8_t tx_packet[RingNetworkProtocol::packet_maxsize];
  volatile uint32_t tx_packet_size;
  volatile uint32_t tx_packet_idx;
  inline bool txIsIdle() { return tx_state == TxState::TxIdle; }

  //data for rx
  enum RxState
  {
    RxIdle,
    ReceivePacketHeader,
    ReceivePacketData,
    ReceivePacketFooter,
    RxEscape,
    ReceiveEndByte
  };
  volatile RxState rx_state;
  volatile RxState rx_state_return;
  volatile uint32_t rx_bytes_to_read;
  volatile uint8_t* rx_bytes_dst;
  //TODO Understand if volatile here is needed, it spreads to read functions and i don't want it
  RingPacket rx_packet;
  volatile bool rx_packet_ready;

  void rxIrq();
  void txIrq();
};

#endif