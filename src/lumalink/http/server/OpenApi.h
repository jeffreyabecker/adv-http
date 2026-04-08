#pragma once
#include "../core/Defines.h"
#if LUMALINK_HTTP_ENABLE_ARDUINO_JSON == 1
#include "OpenApiBuilder.h"
#include "../core/HttpContentTypes.h"
#include "../core/HttpStatus.h"
#include "../handlers/HandlerTypes.h"
#include "../response/StringResponse.h"
#include "WebServerBuilder.h"

#include <string>
#include <string_view>

namespace lumalink::http::openapi
{

	class OpenApiAccessorComponent
	{
	public:
		static constexpr const char *DefaultPath = "/openapi.json";

		explicit OpenApiAccessorComponent(OpenApiBuilder &builder, std::string path = DefaultPath)
			: builder_(builder), path_(std::move(path))
		{
		}

		void operator()(lumalink::http::server::WebServerBuilder &serverBuilder) const
		{
			const lumalink::http::handlers::GetRequest::InvocationWithoutParams handler =
				[this](lumalink::http::core::HttpRequestContext &) -> lumalink::http::handlers::IHttpHandler::HandlerResult
				{
					return lumalink::http::handlers::HandlerResult::responseResult(
						lumalink::http::response::StringResponse::create(
							lumalink::http::core::HttpStatus::Ok(),
							lumalink::http::core::HttpContentTypes::Json,
							builder_.getOpenApiDocument()));
				};

			serverBuilder.handlers().on<lumalink::http::handlers::GetRequest>(
				path_.c_str(),
				handler);
		}

		[[nodiscard]] const std::string &path() const noexcept
		{
			return path_;
		}

	private:
		OpenApiBuilder &builder_;
		std::string path_;
	};

	inline OpenApiAccessorComponent OpenApi(OpenApiBuilder &builder, std::string path = OpenApiAccessorComponent::DefaultPath)
	{
		return OpenApiAccessorComponent(builder, std::move(path));
	}
}

#endif