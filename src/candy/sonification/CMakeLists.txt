add_library(sonification OBJECT
    Sonification.cc
    Sonification.h
    SolverSonification.cc
    SolverSonification.h
    ControllerInterface.cc
    ControllerInterface.h
    oscpack/ip/IpEndpointName.cpp
    oscpack/ip/IpEndpointName.h
    oscpack/ip/NetworkingUtils.cpp
    oscpack/ip/NetworkingUtils.h
    oscpack/ip/PacketListener.h
    oscpack/ip/TimerListener.h
    oscpack/ip/UdpSocket.cpp
    oscpack/ip/UdpSocket.h
    oscpack/osc/MessageMappingOscPacketListener.h
    oscpack/osc/OscException.h
    oscpack/osc/OscHostEndianness.h
    oscpack/osc/OscOutboundPacketStream.cpp
    oscpack/osc/OscOutboundPacketStream.h
    oscpack/osc/OscPacketListener.h
    oscpack/osc/OscPrintReceivedElements.cpp
    oscpack/osc/OscPrintReceivedElements.h
    oscpack/osc/OscReceivedElements.cpp
    oscpack/osc/OscReceivedElements.h
    oscpack/osc/OscTypes.cpp
    oscpack/osc/OscTypes.h
)
