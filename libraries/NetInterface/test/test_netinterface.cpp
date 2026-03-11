#include <Arduino.h>
#include <unity.h>
#include <NetClient.h>

using namespace NetInterface;

// Test ConnectionStatus enum
void test_connection_status_enum() {
    TEST_ASSERT_EQUAL_INT(0, static_cast<int>(ConnectionStatus::CLOSED));
    TEST_ASSERT_EQUAL_INT(1, static_cast<int>(ConnectionStatus::LISTEN));
    TEST_ASSERT_EQUAL_INT(4, static_cast<int>(ConnectionStatus::ESTABLISHED));
}

// Mock client for testing
class MockClient {
public:
    bool connected_value = false;
    size_t bytes_written = 0;
    int available_bytes = 0;
    ConnectionStatus current_status = ConnectionStatus::CLOSED;
    
    size_t write(const uint8_t* buf, size_t size) {
        bytes_written += size;
        return size;
    }
    
    int available() { return available_bytes; }
    int read(uint8_t* buf, size_t size) { return 0; }
    void flush() {}
    void stop() { connected_value = false; }
    int status() { return static_cast<int>(current_status); }
    uint8_t connected() { return connected_value ? 1 : 0; }
    IPAddress remoteIP() { return IPAddress(192, 168, 1, 100); }
    uint16_t remotePort() { return 8080; }
    IPAddress localIP() { return IPAddress(192, 168, 1, 1); }
    uint16_t localPort() { return 80; }
    void setTimeout(uint32_t timeout) {}
    uint32_t getTimeout() { return 1000; }
};

void test_client_impl_basic_operations() {
    MockClient mockClient;
    mockClient.connected_value = true;
    mockClient.current_status = ConnectionStatus::ESTABLISHED;
    mockClient.available_bytes = 100;
    
    ClientImpl<MockClient> client(mockClient);
    
    TEST_ASSERT_EQUAL_INT(1, client.connected());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(ConnectionStatus::ESTABLISHED), static_cast<int>(client.status()));
    TEST_ASSERT_EQUAL_INT(100, client.available());
    
    const char* testData = "Hello World";
    size_t bytesWritten = client.write((const uint8_t*)testData, strlen(testData));
    TEST_ASSERT_EQUAL_INT(strlen(testData), bytesWritten);
    
    TEST_ASSERT_EQUAL_INT(8080, client.remotePort());
    TEST_ASSERT_EQUAL_INT(80, client.localPort());
}

void test_client_impl_timeout() {
    MockClient mockClient;
    ClientImpl<MockClient> client(mockClient);
    
    client.setTimeout(5000);
    TEST_ASSERT_EQUAL_INT(1000, client.getTimeout()); // Mock returns 1000
}

void setup() {
    delay(2000); // Wait for serial monitor
    
    UNITY_BEGIN();
    
    RUN_TEST(test_connection_status_enum);
    RUN_TEST(test_client_impl_basic_operations);
    RUN_TEST(test_client_impl_timeout);
    
    UNITY_END();
}

void loop() {
    // Empty loop for tests
}