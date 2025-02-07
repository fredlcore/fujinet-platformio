#ifndef NETWORK_H
#define NETWORK_H

#include <driver/timer.h>

#include <string>
#include <vector>

#include "bus.h"

#include "Protocol.h"
#include "EdUrlParser.h"
#include "networkStatus.h"
#include "status_error_codes.h"
#include "fnjson.h"

/**
 * Number of devices to expose via DRIVEWIRE, becomes 0x71 to 0x70 + NUM_DEVICES - 1
 */
#define NUM_DEVICES 8

/**
 * The size of rx and tx buffers
 */
#define INPUT_BUFFER_SIZE 65535
#define OUTPUT_BUFFER_SIZE 65535
#define SPECIAL_BUFFER_SIZE 256

class drivewireNetwork : public virtualDevice
{

public:
    /**
     * Constructor
     */
    drivewireNetwork();

    /**
     * Destructor
     */
    virtual ~drivewireNetwork();

    /**
     * Called for DRIVEWIRE Command 'O' to open a connection to a network protocol, allocate all buffers,
     */
    virtual void drivewire_open();

    /**
     * Called for DRIVEWIRE Command 'C' to close a connection to a network protocol, de-allocate all buffers,
     */
    virtual void drivewire_close();

    /**
     * DRIVEWIRE Read command
     * Read # of bytes from the protocol adapter specified by the aux1/aux2 bytes, into the RX buffer. If we are short
     * fill the rest with nulls and return ERROR.
     *  
     * @note It is the channel's responsibility to pad to required length.
     */
    virtual void drivewire_read();

    /**
     * DRIVEWIRE Write command
     * Write # of bytes specified by aux1/aux2 from tx_buffer out to DRIVEWIRE. If protocol is unable to return requested
     * number of bytes, return ERROR.
     */
    virtual void drivewire_write();

    /**
     * DRIVEWIRE Status Command. First try to populate NetworkStatus object from protocol. If protocol not instantiated,
     * or Protocol does not want to fill status buffer (e.g. due to unknown aux1/aux2 values), then try to deal
     * with them locally. Then serialize resulting NetworkStatus object to DRIVEWIRE.
     */
    virtual void drivewire_special();

    /**
     * DRIVEWIRE Special, called as a default for any other DRIVEWIRE command not processed by the other drivewire_ functions.
     * First, the protocol is asked whether it wants to process the command, and if so, the protocol will
     * process the special command. Otherwise, the command is handled locally. In either case, either drivewire_complete()
     * or drivewire_error() is called.
     */
    virtual void drivewire_status();

    /**
     * @brief set channel mode, JSON or PROTOCOL
     */
    virtual void drivewire_set_channel_mode();

    /**
     * @brief Called to set prefix
     */
    virtual void drivewire_set_prefix();

    /**
     * @brief Called to get prefix
     */
    virtual void drivewire_get_prefix();

    /**
     * @brief called to set login
     */
    virtual void drivewire_set_login();

    /**
     * @brief called to set password
     */
    virtual void drivewire_set_password();

private:
    /**
     * Buffer for holding devicespec
     */
    uint8_t devicespecBuf[256];

    /**
     * The Receive buffer for this N: device
     */
    string *receiveBuffer = nullptr;

    /**
     * The transmit buffer for this N: device
     */
    string *transmitBuffer = nullptr;

    /**
     * The special buffer for this N: device
     */
    string *specialBuffer = nullptr;

    /**
     * The EdUrlParser object used to hold/process a URL
     */
    EdUrlParser *urlParser = nullptr;

    /**
     * Instance of currently open network protocol
     */
    NetworkProtocol *protocol = nullptr;

    /**
     * Network Status object
     */
    NetworkStatus status;

    /**
     * Devicespec passed to us, e.g. N:HTTP://WWW.GOOGLE.COM:80/
     */
    string deviceSpec;

    /**
     * The currently set Prefix for this N: device, set by DRIVEWIRE call 0x2C
     */
    string prefix;

    /**
     * The AUX1 value used for OPEN.
     */
    uint8_t open_aux1 = 0;

    /**
     * The AUX2 value used for OPEN.
     */
    uint8_t open_aux2 = 0;

    /**
     * The Translation mode ORed into AUX2 for READ/WRITE/STATUS operations.
     * 0 = No Translation, 1 = CR<->EOL (Macintosh), 2 = LF<->EOL (UNIX), 3 = CR/LF<->EOL (PC/Windows)
     */
    uint8_t trans_aux2 = 0;

    /**
     * Return value for DSTATS inquiry
     */
    uint8_t inq_dstats=0xFF;

    /**
     * The login to use for a protocol action
     */
    string login;

    /**
     * The password to use for a protocol action
     */
    string password;

    /**
     * The channel mode for the currently open DRIVEWIRE device. By default, it is PROTOCOL, which passes
     * read/write/status commands to the protocol. Otherwise, it's a special mode, e.g. to pass to
     * the JSON or XML parsers.
     * 
     * @enum PROTOCOL Send to protocol
     * @enum JSON Send to JSON parser.
     */
    enum _channel_mode
    {
        PROTOCOL,
        JSON
    } channelMode;

    /**
     * saved NetworkStatus items
     */
    unsigned char reservedSave = 0;
    unsigned char errorSave = 1;

    /**
     * The fnJSON parser wrapper object
     */
    FNJSON *json = nullptr;

    /**
     * Bytes sent of current JSON query object.
     */
    unsigned short json_bytes_remaining=0;

    /**
     * Instantiate protocol object
     * @return bool TRUE if protocol successfully called open(), FALSE if protocol could not open
     */
    bool instantiate_protocol();

    /**
     * Is this a valid URL? (used to generate ERROR 165)
     */
    bool isValidURL(EdUrlParser *url);

    /**
     * Preprocess a URL given aux1 open mode. This is used to work around various assumptions that different
     * disk utility packages do when opening a device, such as adding wildcards for directory opens. 
     * 
     * The resulting URL is then sent into EdURLParser to get our URLParser object which is used in the rest
     * of drivewireNetwork.
     * 
     * This function is a mess, because it has to be, maybe we can factor it out, later. -Thom
     */
    bool parseURL();

    /**
     * We were passed a COPY arg from DOS 2. This is complex, because we need to parse the comma,
     * and figure out one of three states:
     * 
     * (1) we were passed D1:FOO.TXT,N:FOO.TXT, the second arg is ours.
     * (2) we were passed N:FOO.TXT,D1:FOO.TXT, the first arg is ours.
     * (3) we were passed N1:FOO.TXT,N2:FOO.TXT, get whichever one corresponds to our device ID.
     * 
     * DeviceSpec will be transformed to only contain the relevant part of the deviceSpec, sans comma.
     */
    void processCommaFromDevicespec();

    /**
     * Perform the correct read based on value of channelMode
     * @param num_bytes Number of bytes to read.
     * @return TRUE on error, FALSE on success. Passed directly to bus_to_computer().
     */
    bool drivewire_read_channel(unsigned short num_bytes);

    /**
     * @brief Perform read of the current JSON channel
     * @param num_bytes Number of bytes to read
     */
    bool drivewire_read_channel_json(unsigned short num_bytes);

    /**
     * Perform the correct write based on value of channelMode
     * @param num_bytes Number of bytes to write.
     * @return TRUE on error, FALSE on success. Used to emit drivewire_error or drivewire_complete().
     */
    bool drivewire_write_channel(unsigned short num_bytes);

    /**
     * @brief perform local status commands, if protocol is not bound, based on cmdFrame
     * value.
     */
    void drivewire_status_local();

    /**
     * @brief perform channel status commands, if there is a protocol bound.
     */
    void drivewire_status_channel();

    /**
     * @brief get JSON status (# of bytes in receive channel)
     */
    bool drivewire_status_channel_json(NetworkStatus *ns);

    /**
     * @brief Do an inquiry to determine whether a protoocol supports a particular command.
     * The protocol will either return $00 - No Payload, $40 - Atari Read, $80 - Atari Write,
     * or $FF - Command not supported, which should then be used as a DSTATS value by the
     * Atari when making the N: DRIVEWIRE call.
     */
    void drivewire_special_inquiry();

    /**
     * @brief called to handle special protocol interactions when DSTATS=$00, meaning there is no payload.
     * Essentially, call the protocol action 
     * and based on the return, signal drivewire_complete() or error().
     */
    void drivewire_special_00();

    /**
     * @brief called to handle protocol interactions when DSTATS=$40, meaning the payload is to go from
     * the peripheral back to the ATARI. Essentially, call the protocol action with the accrued special
     * buffer (containing the devicespec) and based on the return, use bus_to_computer() to transfer the
     * resulting data. Currently this is assumed to be a fixed 256 byte buffer.
     */
    void drivewire_special_40();

    /**
     * @brief called to handle protocol interactions when DSTATS=$80, meaning the payload is to go from
     * the ATARI to the pheripheral. Essentially, call the protocol action with the accrued special
     * buffer (containing the devicespec) and based on the return, use bus_to_peripheral() to transfer the
     * resulting data. Currently this is assumed to be a fixed 256 byte buffer.
     */
    void drivewire_special_80();

    /**
     * @brief Perform the inquiry, handle both local and protocol commands.
     * @param inq_cmd the command to check against.
     */
    void do_inquiry(unsigned char inq_cmd);

    /**
     * @brief set translation specified by aux1 to aux2_translation mode.
     */
    void drivewire_set_translation();

    /**
     * @brief Parse incoming JSON. (must be in JSON channelMode)
     */
    void drivewire_parse_json();

    /**
     * @brief Set JSON query string. (must be in JSON channelMode)
     */
    void drivewire_set_json_query();

    /**
     * @brief Set timer rate for PROCEED timer in ms
     */
    void drivewire_set_timer_rate();

    /**
     * @brief perform ->FujiNet commands on protocols that do not use an explicit OPEN channel.
     */
    void drivewire_do_idempotent_command_80();

    /**
     * @brief parse URL and instantiate protocol
     */
    void parse_and_instantiate_protocol();

};

#endif /* NETWORK_H */
