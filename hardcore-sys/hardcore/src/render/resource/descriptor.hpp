#pragma once

#include <render/descriptor.h>

namespace hc::render::resource {
	Sz size_of(HCPrimitive primitive);

	class Descriptor {
	public:
		explicit Descriptor(const HCDescriptor &descriptor);

		[[nodiscard]] Sz size() const noexcept;

	private:
		std::vector<HCField> fields;
		HCAlignment alignment;
	};
}
