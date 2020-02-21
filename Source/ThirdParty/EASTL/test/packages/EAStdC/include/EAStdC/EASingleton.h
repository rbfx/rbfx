///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EASTDC_EASINGLETON_H
#define EASTDC_EASINGLETON_H

#include <EAStdC/internal/Config.h>
#include <EAAssert/eaassert.h>


#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4127) // conditional expression is constant
#endif


/// The standard Electronic Arts namespace.
namespace EA
{
namespace StdC
{
	/// \class Singleton
	///
	/// Singleton adds singleton semantics to derived classes.  It provides
	/// singleton-style instance accessors and will assert if more than one
	/// instance of the derived class is constructed.
	///
	/// \code
	/// class UniqueWidget : public EA::StdC::Singleton<UniqueWidget> { };
	/// UniqueWidget *pWidget = UniqueWidget::GetInstance();
	/// \endcode
	///
	/// \param  T           The classname from which to create the singleton.
	/// \param  kId         Multiple unique singleton instances of class \a T
	///                     can be created if they are given unique \a kId
	///                     numbers.
	///
	template <typename T, unsigned int kId = 0>
	class Singleton
	{
	public:
		typedef T value_type;
		typedef Singleton<T, kId> this_type;

		/// Return the pointer to the singleton instance.
		static T* GetInstance()
		{
			return static_cast<T*>(spInstance);
		}

	protected:
		/// Constructor
		/// The singleton instance is assigned at construction time.
		Singleton()
		{
			EA_ASSERT_FORMATTED(!spInstance, ("Singleton instance (%p) has already been created", static_cast<T*>(spInstance)));
			spInstance = this;
		}

		/// Destructor
		/// This destructor is intentionally not marked 'virtual'.  We don't
		/// want to force virtual-ness on our derived class.
		~Singleton()
		{
			spInstance = NULL;
		}

	private:
		/// Private (disabled) copy constructor
		Singleton(const this_type&);

		/// Private (disabled) assignment operator
		Singleton & operator=(const this_type&);

		/// Static pointer to this singleton's instance.
		static this_type* spInstance;
	};


	/// \class  SingletonAdapter
	///
	/// SingletonAdapter adds singleton semantics to an existing class by
	/// extending its public interface.  This is useful for creating
	/// singletons from existing (and potentially externally maintained)
	/// classes without modifiying the original code directly.
	///
	/// To use this class, derive a new class with a unique name from the base
	/// class (since the static instance pointer will be unique only for this
	/// class name).
	///
	/// \code
	/// class WidgetSingleton : public EA::StdC::SingletonAdapter<Widget> { };
	/// \endcode
	///
	/// If implicit creation was requested, the singleton's instance will be
	/// created the first time it is accessed.  Otherwise, CreateInstance() or
	/// SetInstance() must be called.
	///
	/// The singleton's instance can be destructed and freed using Destroy().
	///
	/// SingletonAdapter<> is not thread-safe.
	///
	/// If you're going to be using \a T exclusively through the
	/// SingletonAdapter interface, you should consider making its constructor
	/// and destructor protected members.  That will ensure that instances of
	/// \a T can only be created by the SingletonAdapter layer.
	///
	/// \param  T           The classname from which to create the singleton.
	/// \param  bImplicitCreation   If implicit creation is requested, the
	///                     singleton instance will be created on the first
	///                     attempt to access it.
	/// \param  kId         Multiple unique singleton instances of class \a T
	///                     can be created if they are given unique \a kId
	///                     numbers.
	///
	template <typename T, bool bImplicitCreation = false, unsigned int kId = 0>
	class SingletonAdapter : public T
	{
	public:
		typedef T value_type;
		typedef SingletonAdapter<T, bImplicitCreation, kId> this_type;

		/// Return the pointer to the singleton instance.
		/// If \a bImplicitCreation was requested, an instance will be created
		/// if one does not already exist.
		static T* GetInstance()
		{
			if (bImplicitCreation && !spInstance)
				return CreateInstance(EASTDC_ALLOC_PREFIX "SingletonAdapter");

			return static_cast<T*>(spInstance);
		}

		/// This allows you to manually set the instance, which is useful
		/// if you want to allocate the memory for it yourself. 
		/// \return A pointer to the previous instance.
		static T* SetInstance(T* pInstance)
		{
			T* const pPrevious = spInstance;
			spInstance = pInstance;
			return pPrevious;
		}

		/// Create this singleton's instance.  If the instance has already
		/// been created, a pointer to it will simply be returned.
		/// \return A pointer to the singleton's instance.
		static T* CreateInstance(const char* pName)
		{
			if (!spInstance)
				spInstance = EASTDC_NEW(pName) SingletonAdapter;

			return spInstance;
		}

		/// Destroy this singleton's instance.
		static void DestroyInstance()
		{
			if (spInstance)
			{
				EASTDC_DELETE spInstance;
				spInstance = NULL;
			}
		}

	protected:
		/// Constructor
		SingletonAdapter() { }

		/// Destructor
		~SingletonAdapter() { }

	private:
		/// Private (disabled) copy constructor
		SingletonAdapter(const this_type&);

		/// Private (disabled) assignment operator
		SingletonAdapter& operator=(const this_type&);

		/// Static pointer to this singleton's instance.
		static this_type* spInstance;
	};

} // namespace StdC
} // namespace EA


// Initialize the singletons' static instance pointers to NULL.
template <typename T, unsigned int kId> EA::StdC::Singleton<T, kId> *EA::StdC::Singleton<T, kId>::spInstance = NULL;
template <typename T, bool bImplicitCreation, unsigned int kId> EA::StdC::SingletonAdapter<T, bImplicitCreation, kId> *EA::StdC::SingletonAdapter<T, bImplicitCreation, kId>::spInstance = NULL;


#ifdef _MSC_VER
	#pragma warning(pop)
#endif


#endif // Header include guard
