#pragma once

#include "CommonTypes.h"
#include <memory>

struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;

namespace DK
{
	template<typename T> using Ref = std::shared_ptr<T>;

	template<typename T, typename... Args> dk_inline Ref<T> makeRef(Args&&... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

#define DEFINE_REFCOUNTED_STRCUT(name) \
	struct name; \
	using name##Ref = Ref<name>;
#define DEFINE_REFCOUNTED(name) \
	class name; \
	using name##Ref = Ref<name>;
	DEFINE_REFCOUNTED_STRCUT(IBuffer);
	DEFINE_REFCOUNTED_STRCUT(AppearanceData);
	DEFINE_REFCOUNTED(ModelProperty);
	DEFINE_REFCOUNTED(StaticMeshModel);
	DEFINE_REFCOUNTED(SkinnedMeshModel);
	DEFINE_REFCOUNTED(Skeleton);
	DEFINE_REFCOUNTED(Animation);
	DEFINE_REFCOUNTED(ITexture);

	using VertexBufferViewRef = Ref<D3D12_VERTEX_BUFFER_VIEW>;
	using IndexBufferViewRef = Ref<D3D12_INDEX_BUFFER_VIEW>;

	template <typename T>
	class Ptr
	{
	public:
		~Ptr()
		{
			release();
		}
		Ptr()
			: _ptr(nullptr)
		{}
		Ptr(T* ptr)
			: _ptr(ptr)
		{}
		Ptr(Ptr&& rhs) : _ptr(rhs._ptr)
		{
			rhs._ptr = nullptr;
		}

		dk_inline const Ptr& operator=(Ptr&& rhs)
		{
			if (this != &rhs)
			{
				release();

				_ptr = rhs._ptr;
				rhs._ptr = nullptr;
			}

			return *this;
		}
		dk_inline T* operator->() noexcept
		{
			return _ptr;
		}
		dk_inline const T* operator->() const noexcept
		{
			return _ptr;
		}

		dk_inline void assign(T* ptr) noexcept
		{
			if (_ptr != ptr)
			{
				release();
				_ptr = ptr;
			}
		}
		dk_inline T* relocate() noexcept
		{
			T* ptr = _ptr;
			_ptr = nullptr;
			return ptr;
		}
		dk_inline void release() noexcept
		{
			//DK_LOG("Ptr Delete: Owner=%p Object=%p", this, _ptr);

			T* ptr = _ptr;
			_ptr = nullptr;

			if (ptr)
				dk_delete ptr;
		}
		dk_inline T* get() noexcept
		{
			return _ptr;
		}
		dk_inline const T* get() const noexcept
		{
			return _ptr;
		}
		dk_inline void swap(Ptr& rhs) noexcept
		{
			T* temp = rhs._ptr;
			rhs._ptr = _ptr;
			_ptr = temp;
		}

	private:
		T* _ptr;
	};

	template <typename T>
	class RenderResourcePtr
	{
	public:
		dk_inline ~RenderResourcePtr()
		{
			release();
		}
		dk_inline RenderResourcePtr()
		{}
		dk_inline RenderResourcePtr(T* ptr)
			: _ptr(ptr)
		{}
		RenderResourcePtr(RenderResourcePtr&& rhs)
		{
			release();
			operator=(DK::move(rhs));
		}
		dk_inline const RenderResourcePtr& operator=(RenderResourcePtr&& rhs)
		{
			if (this != &rhs)
			{
				release();
				_ptr = rhs._ptr;
				rhs._ptr = nullptr;
			}

			return *this;
		}

		void release()
		{
			if (_ptr != nullptr)
				_ptr->Release();
			_ptr = nullptr;
		}

		dk_inline operator T*()
		{
			return _ptr;
		}

		dk_inline T* operator->() noexcept
		{
			return _ptr;
		}
		dk_inline const T* operator->() const noexcept
		{
			return _ptr;
		}

		dk_inline T* get() noexcept
		{
			return _ptr;
		}
		dk_inline const T* get() const noexcept
		{
			return _ptr;
		}

	private:
		T* _ptr = nullptr;
	};
}
