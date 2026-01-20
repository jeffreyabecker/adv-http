#pragma once
#include <Arduino.h>
#include <memory>
#include <functional>
#include "./HttpResponse.h"
namespace HttpServerAdvanced::Core
{
    

    // class HttpResponseFilters
    // {
    // private:
    //     std::vector<IHttpResponse::ResponseFilter> filters_;
    //     HttpResponse::ResponseFilter curry(HttpResponse::ResponseFilter filter)
    //     {
    //         return [filter](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse>
    //         {
    //             return filter(std::move(resp));
    //         };
    //     }
    // public:
    //     HttpResponseFilters() = default;
    //     ~HttpResponseFilters() = default;
    //     static constexpr const char *ServiceName = "HttpResponseFilters";

    //     void add(HttpResponse::ResponseFilter filter)
    //     {
    //         filters_.emplace_back(filter);
    //     }

    //     std::unique_ptr<IHttpResponse> apply(std::unique_ptr<IHttpResponse> response)
    //     {
    //         for (auto &filter : filters_)
    //         {
    //             response = filter(std::move(response));
    //         }
    //         return response;
    //     }
    //     auto getApplier()
    //     {
    //         return curry([this](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse>
    //         {
    //             return this->apply(std::move(resp));
    //         });
    //     }
    // };

    HttpResponse::ResponseFilter CrossOriginRequestSharing(const String &allowedOrigins = "*", const String &allowedMethods = "*", const String &allowedHeaders = "*", 
        const String &allowedCredentials = "", const String &exposeHeaders = "", const int maxAge = -1, const String & requestHeaders = "", const String & requestMethods = "")
    {
        return [=](std::unique_ptr<IHttpResponse> resp) -> std::unique_ptr<IHttpResponse>
        {
            if(!resp)
            {
                return resp;
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_ALLOW_ORIGIN) && !allowedOrigins.isEmpty()){
                resp->headers().set(HttpHeaders::AccessControlAllowOrigin( allowedOrigins));
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_ALLOW_METHODS) && !allowedMethods.isEmpty()){
                resp->headers().set(HttpHeaders::AccessControlAllowMethods( allowedMethods));
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_ALLOW_HEADERS) && !allowedHeaders.isEmpty()){
                resp->headers().set(HttpHeaders::AccessControlAllowHeaders( allowedHeaders));
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_ALLOW_CREDENTIALS) && !allowedCredentials.isEmpty()){
                resp->headers().set(HttpHeaders::AccessControlAllowCredentials(allowedCredentials));
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_EXPOSE_HEADERS) && !exposeHeaders.isEmpty()){
                resp->headers().set(HttpHeaders::AccessControlExposeHeaders(exposeHeaders));
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_MAX_AGE) && maxAge >= 0){
                resp->headers().set(HttpHeaders::AccessControlMaxAge(String(maxAge)));
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_REQUEST_HEADERS) && !requestHeaders.isEmpty()){
                resp->headers().set(HttpHeaders::AccessControlRequestHeaders(requestHeaders));
            }
            if(!resp->headers().exists(HttpHeaders::ACCESS_CONTROL_REQUEST_METHOD) && !requestMethods.isEmpty()){
                resp->headers().set(HttpHeaders::AccessControlRequestMethod(requestMethods));
            }
            return resp;
        };
    }


} // namespace HttpServerAdvanced::Core