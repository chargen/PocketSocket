//  Copyright 2014 Zwopple Limited
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#import <Foundation/Foundation.h>
#import "PSWebSocketTypes.h"

typedef struct PSWebSocketByteCount
{
	uint64_t bytes;
	uint64_t numberOf64BitOverflows; // ULONG_LONG_MAX
}
PSWebSocketByteCount; // total bytes are (numberOf64BitOverflows * ULONG_LONG_MAX + bytes)

void PSWebSocketAddBytesToByteCount(uint64_t bytes, PSWebSocketByteCount *byteCount);

typedef NS_ENUM(NSInteger, PSWebSocketReadyState) {
    PSWebSocketReadyStateConnecting = 0,
    PSWebSocketReadyStateOpen,
    PSWebSocketReadyStateClosing,
    PSWebSocketReadyStateClosed
};

@class PSWebSocket;

/**
 *  PSWebSocketDelegate
 */
@protocol PSWebSocketDelegate <NSObject>

@required
- (void)webSocketDidOpen:(PSWebSocket *)webSocket;
- (void)webSocket:(PSWebSocket *)webSocket didFailWithError:(NSError *)error;
- (void)webSocket:(PSWebSocket *)webSocket didReceiveMessage:(id)message;
- (void)webSocket:(PSWebSocket *)webSocket didCloseWithCode:(NSInteger)code reason:(NSString *)reason wasClean:(BOOL)wasClean;

@optional
- (BOOL)webSocket:(PSWebSocket *)webSocket shouldTrustServer:(SecTrustRef)serverTrust;

@end

/**
 *  PSWebSocket
 */
@interface PSWebSocket : NSObject

#pragma mark - Class Methods

/**
 *  Given a NSURLRequest determine if it is a websocket request based on it's headers
 *
 *  @param request request to check
 *
 *  @return whether or not the given request is a websocket request
 */
+ (BOOL)isWebSocketRequest:(NSURLRequest *)request;

#pragma mark - Properties

@property (nonatomic, assign, readonly) PSWebSocketReadyState readyState;
@property (nonatomic, weak) id <PSWebSocketDelegate> delegate;
@property (nonatomic, strong) dispatch_queue_t delegateQueue;
@property (nonatomic, assign, readonly) PSWebSocketByteCount bytesSent;
@property (nonatomic, assign, readonly) PSWebSocketByteCount bytesReceived;

#pragma mark - Initialization

/**
 *  Initialize a PSWebSocket instance in client mode.
 *  Calls internally clientSocketWithRequest:withStreamSecurityOptions:withStreamProperties:
 *
 *  @param request that is to be used to initiate the handshake
 *
 *  @return an initialized instance of PSWebSocket in client mode
 */

+ (instancetype)clientSocketWithRequest:(NSURLRequest *)request;

/**
 *  Initialize a PSWebSocket instance in server mode
 *
 *  @param request      request that is to be used to initiate the handshake response
 *  @param inputStream  opened input stream to be taken over by the websocket
 *  @param outputStream opened output stream to be taken over by the websocket
 *
 *  @return an initialized instance of PSWebSocket in server mode
 */
+ (instancetype)serverSocketWithRequest:(NSURLRequest *)request inputStream:(NSInputStream *)inputStream outputStream:(NSOutputStream *)outputStream;

#pragma mark - Actions

/**
 *  Opens the websocket connection and initiates the handshake. Once
 *  opened an instance of PSWebSocket can never be opened again. The
 *  connection obeys any timeout interval set on the NSURLRequest used
 *  to initialize the websocket.
 */
- (void)open;

/**
 *  Send a message over the websocket
 *
 *  @param message an instance of NSData or NSString to send
 */
- (void)send:(id)message;

/**
 *  Send a ping over the websocket
 *
 *  @param pingData data to include with the ping
 *  @param handler  optional callback handler when the corrosponding pong is received
 */
- (void)ping:(NSData *)pingData handler:(void (^)(NSData *pongData))handler;


/**
 *  Close the websocket will default to code 1000 and nil reason
 */
- (void)close;

/**
 *  Close the websocket with a specific code and/or reason
 *
 *  @param code   close code reason
 *  @param reason short textual reason why the connection was closed
 */
- (void)closeWithCode:(NSInteger)code reason:(NSString *)reason;

#pragma mark - Stream Properties

/**
 *  Copy a property from the streams this websocket is backed by
 *
 *  @param key property key - see kCFStreamProperty constants
 *
 *  @return property value
 */
- (CFTypeRef)copyStreamPropertyForKey:(NSString *)key;

/**
 *  Set a property on the streams this websocket is backed by. Calling this
 *  method once the websocket has been opened will raise an exception.
 *
 *  @param property property value
 *  @param key      property key - see kCFStreamProperty constants
 */
- (void)setStreamProperty:(CFTypeRef)property forKey:(NSString *)key;

#pragma mark - Advanced stream options

/**
 *  Set option to use user certificate checking. Calling this
 *  method once the websocket has been opened will raise an exception.
 *
 *  @param userCertificateChecking	Use user certificate checking.
 *
 *  @warning Defaults to NO. If set to YES websocket delegate should implement 
 *	(webSocket:shouldTrustServer:) method. If this method returns NO websocket
 *	connection is closed with SSL error.
 */
- (void)setShouldUseStrictUserCertificateChecking:(BOOL)strictUserCertificateChecking;

/**
 *  Set ssl options dictionary on the streams this websocket is backed by. Calling this
 *  method once the websocket has been opened will raise an exception.
 *
 *  @param sslOptions	NSDictionary of ssl options for the stream
 *
 *  @warning sslOptions should be set in this method and not by setStreamProperty:forKey:
 *	since this method handles proxy case properly. 
 *	If you set ssl options in setStreamProperty:forKey: proxy case will not be handled properly!
 */
- (void)setSSLOptions:(NSDictionary *)sslOptions;

/**
 *  Set enabled ciphers on the streams this websocket is backed by. Calling this
 *  method once the websocket has been opened will raise an exception.
 *
 *  @param ciphers	allocated in heap array of ciphers, websocket take ownership of it
 *  @param count    number of ciphers in array
 *
 *  @warning Settings set in this method will be applied only
 *  when 'open' method will be called. This means it potentially can
 *  override settings set by other methods.
 */
- (void)setEnabledCiphers:(SSLCipherSuite *)ciphers count:(size_t)count;

/**
 *  Set SSL protocol minimum version for streams this websocket is backed by. Calling this
 *  method once the websocket has been opened will raise an exception.
 *
 *  @param minVersion	minimum version of SSLProtocol
 *  
 *  @warning Settings set in this method will be applied only 
 *  when 'open' method will be called. This means it potentially can
 *  override settings set by other methods. This is especially the case 
 *  if you have set kCFStreamSSLLevel in ssl settings dictionary set by
 *  - (void)setStreamProperty:(CFTypeRef)property forKey:(NSString *)key;
 *  with key kCFStreamPropertySSLSettings
 *
 */
- (void)setSSLSetProtocolVersionMin:(SSLProtocol)minVersion;

/**
 *  Set SSL protocol maximum version for streams this websocket is backed by. Calling this
 *  method once the websocket has been opened will raise an exception.
 *
 *  @param maxVersion	maximum version of SSLProtocol
 *
 *  @warning Settings set in this method will be applied only
 *  when 'open' method will be called. This means it potentially can
 *  override settings set by other methods. This is especially the case
 *  if you have set kCFStreamSSLLevel in ssl settings dictionary set by
 *  - (void)setStreamProperty:(CFTypeRef)property forKey:(NSString *)key;
 *  with key kCFStreamPropertySSLSettings
 */
- (void)setSSLSetProtocolVersionMax:(SSLProtocol)maxVersion;

#pragma mark - Statistics

/**
 *  Resets byte counts to zeros
 */
- (void)resetByteCounts;

@end
