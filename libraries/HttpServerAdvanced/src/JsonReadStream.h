// #pragma once
// #include <ArduinoJson.h>

// namespace HttpServerAdvanced
// {

//     template <size_t N>
//     class JsonReadStream : public Stream
//     {
//     private:
//         struct InnerBuffer
//         {
//             uint8_t buffer[N];
//             size_t bufferOperationPosition;
//             size_t logicalPosition;
//             size_t readPosition; // the read position within buffer
//             size_t available(){
//                 return N - readPosition;
//             }

//             size_t write(uint8_t b)
//             {
//                 if (bufferOperationPosition < logicalPosition)
//                 {
//                     bufferOperationPosition++;
//                     return 1;
//                 }
//                 if (bufferOperationPosition - logicalPosition < N)
//                 {
//                     buffer[bufferOperationPosition - logicalPosition] = b;
//                     bufferOperationPosition++;
//                     return 1;
//                 }
//                 return 0;
//             }
//             size_t write(const uint8_t *buffer, size_t size)
//             {
//                 size_t n = 0;
//                 while (size--)
//                 {
//                     if (write(*buffer++))
//                         n++;
//                     else
//                         break;
//                 }
//                 return n;
//             }
//         };
 

//         JsonDocument doc_; // the JsonDocument to serialize from
//         size_t totalSerializedLength_;
//         InnerBuffer buffer_;

//         // the idea here is that whenever there's a buffer underflow, we're gonna
//         // reserialize the document passing this as the writer. if
//         void bufferData(){
//             buffer_.bufferOperationPosition = 0;
//             buffer_.readPosition = 0;
//             size_t serializedCount = serializeJson(doc_, buffer_);
//         }
//         inline size_t currentLogicalPosition(){ return buffer_.logicalPosition + buffer_.readPosition;}
//     public:
//         JsonReadStream(JsonDocument && doc)
//             : doc_(std::move(doc)), totalSerializedLength_(measureJson(doc))
//         {
//             bufferData();
//         }

//         virtual int available() override{
//             return totalSerializedLength_ - currentLogicalPosition();
//         }

//         virtual int read() override{
//             int result = peek();
//             if(result >0){
//                 buffer_.readPosition++;                
//             }
//             return result;
//         }

//         virtual int peek() override{
//             if(currentLogicalPosition() >= totalSerializedLength_){
//                 return -1;
//             }
//             if(buffer_.available() == 0){
//                 bufferData();
//             }
//             return buffer_.buffer[buffer_.readPosition];
//         }

//         virtual size_t write(uint8_t b) override{
//             return 0;
//         }
//     };

// }