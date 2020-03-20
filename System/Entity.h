#ifndef _RTEENTITY_
#define _RTEENTITY_

#include "Serializable.h"
#include "RTEError.h"

namespace RTE {

	typedef std::function<void*()> MemoryAllocate; //!< Convenient name definition for the memory allocation callback function.
	typedef std::function<void(void*)> MemoryDeallocate; //!< Convenient name definition for the memory deallocation callback function.

#pragma region Global Macro Definitions
	#define ABSTRACTCLASSINFO(TYPE, PARENT)	\
		Entity::ClassInfo TYPE::m_sClass(#TYPE, &PARENT::m_sClass);

	#define CONCRETECLASSINFO(TYPE, PARENT, BLOCKCOUNT) \
		Entity::ClassInfo TYPE::m_sClass(#TYPE, &PARENT::m_sClass, TYPE::Allocate, TYPE::Deallocate, TYPE::NewInstance, BLOCKCOUNT);

	#define CONCRETESUBCLASSINFO(TYPE, SUPER, PARENT, BLOCKCOUNT) \
		Entity::ClassInfo SUPER::TYPE::m_sClass(#TYPE, &PARENT::m_sClass, SUPER::TYPE::Allocate, SUPER::TYPE::Deallocate, SUPER::TYPE::NewInstance, BLOCKCOUNT);

	/// <summary>
	/// Convenience macro to cut down on duplicate ClassInfo methods in classes that extend Entity.
	/// </summary>
	#define CLASSINFOGETTERS \
		const Entity::ClassInfo & GetClass() const { return m_sClass; } \
		const std::string & GetClassName() const { return m_sClass.GetName(); }

	/// <summary>
	/// Static method used in conjunction with ClassInfo to allocate an Entity. 
	/// This function is passed into the constructor of this Entity's static ClassInfo's constructor, so that it can instantiate MovableObjects.
	/// </summary>
	/// <returns>A pointer to the newly dynamically allocated Entity. Ownership is transferred as well.</returns>
	#define ENTITYALLOCATION(TYPE)																		\
		static void * operator new (size_t size) { return TYPE::m_sClass.GetPoolMemory(); }				\
		static void operator delete (void *pInstance) { TYPE::m_sClass.ReturnPoolMemory(pInstance); }	\
		static void * operator new (size_t size, void *p) throw() { return p; }							\
		static void operator delete (void *, void *) throw() {  }										\
		static void * Allocate() { return malloc(sizeof(TYPE)); }										\
		static void Deallocate(void *pInstance) { free(pInstance); }									\
		static Entity * NewInstance() { return new TYPE; }												\
		virtual Entity * Clone(Entity *pCloneTo = 0) const {											\
			TYPE *pEnt = pCloneTo ? dynamic_cast<TYPE *>(pCloneTo) : new TYPE();						\
			RTEAssert(pEnt, "Tried to clone to an incompatible instance!");								\
			if (pCloneTo) { pEnt->Destroy(); }															\
			pEnt->Create(*this);																		\
			return pEnt;																				\
		}
#pragma endregion

	/// <summary>
	/// Whether to draw the colors, or own material property, or to clear the corresponding non-key-color pixels of the Entity being drawn with key-color pixels on the target.
	/// </summary>
	enum DrawMode {
		g_DrawColor = 0,
		g_DrawMaterial,
		g_DrawAir,
		g_DrawKey,
		g_DrawWhite,
		g_DrawMOID,
		g_DrawNoMOID,
		g_DrawDebug,
		g_DrawLess,
		g_DrawTrans,
		g_DrawRedTrans,
		g_DrawScreen,
		g_DrawAlpha
	};

	/// <summary>
	/// The base class that specifies certain common creation/destruction patterns and simple reflection support for virtually all RTE classes.
	/// </summary>
	class Entity : public Serializable {
		friend class DataModule;

	public:

#pragma region ClassInfo
		/// <summary>
		/// The class that describes each subclass of Entity. There should be one ClassInfo static instance for every Entity child.
		/// </summary>
		class ClassInfo {
			friend class Entity;

		public:

#pragma region Creation
			/// <summary>
			/// Constructor method used to instantiate a ClassInfo Entity.
			/// </summary>
			/// <param name="name">A friendly-formatted name of the Entity that is going to be represented by this ClassInfo.</param>
			/// <param name="pParentInfo">Pointer to the parent class' info. 0 if this describes a root class.</param>
			/// <param name="fpAllocFunc">Function pointer to the raw allocation function of the derived's size. If the represented Entity subclass isn't concrete, pass in 0.</param>
			/// <param name="fpDeallocFunc">Function pointer to the raw deallocation function of memory. If the represented Entity subclass isn't concrete, pass in 0.</param>
			/// <param name="fpNewFunc">Function pointer to the new instance factory. If the represented Entity subclass isn't concrete, pass in 0.</param>
			/// <param name="allocBlockCount">The number of new instances to fill the pre-allocated pool with when it runs out.</param>
			ClassInfo(const std::string &name, ClassInfo *pParentInfo = 0, MemoryAllocate fpAllocFunc = 0, MemoryDeallocate fpDeallocFunc = 0, Entity * (*fpNewFunc)() = 0, int allocBlockCount = 10);
#pragma endregion

#pragma region Getters
			/// <summary>
			/// Gets the name of this ClassInfo.
			/// </summary>
			/// <returns>A string with the friendly-formatted name of this ClassInfo.</returns>
			const std::string & GetName() const { return m_Name; }

			/// <summary>
			/// Gets the names of all ClassInfos in existence.
			/// </summary>
			/// <returns>A list of the names.</returns>
			static plf::list<std::string> GetClassNames();

			/// <summary>
			/// Gets the ClassInfo of a particular RTE class corresponding to a friendly-formatted string name. 
			/// </summary>
			/// <param name="name">The friendly name of the desired ClassInfo.</param>
			/// <returns>A pointer to the requested ClassInfo, or 0 if none that matched the name was found. Ownership is NOT transferred!</returns>
			static const ClassInfo * GetClass(const std::string &name);

			/// <summary>
			/// Gets the ClassInfo which describes the parent of this.
			/// </summary>
			/// <returns>A pointer to the parent ClassInfo. 0 if this is a root class.</returns>
			const ClassInfo * GetParent() const { return m_pParentInfo; }
#pragma endregion

#pragma region Memory Management
			/// <summary>
			/// Grabs from the pre-allocated pool, an available chunk of memory the exact size of the Entity this ClassInfo represents. OWNERSHIP IS TRANSFERRED!
			/// </summary>
			/// <returns>A pointer to the pre-allocated pool memory. OWNERSHIP IS TRANSFERRED!</returns>
			virtual void * GetPoolMemory();

			/// <summary>
			/// Returns a raw chunk of memory back to the pre-allocated available pool.
			/// </summary>
			/// <param name="pReturnedMemory">The raw chunk of memory that is being returned. Needs to be the same size as the type this ClassInfo describes. OWNERSHIP IS TRANSFERRED!</param>
			/// <returns>The count of outstanding memory chunks after this was returned.</returns>
			virtual int ReturnPoolMemory(void *pReturnedMemory);

			/// <summary>
			/// Writes a bunch of useful debug info about the memory pools to a file.
			/// </summary>
			/// <param name="fileWriter">The writer to write info to.</param>
			static void DumpPoolMemoryInfo(Writer &fileWriter);

			/// <summary>
			/// Adds a certain number of newly allocated instances to this' pool.
			/// </summary>
			/// <param name="fillAmount">The number of instances to fill the pool with. If 0 is specified, the set refill amount will be used.</param>
			void FillPool(int fillAmount = 0);

			/// <summary>
			/// Adds a certain number of newly allocated instances to all pools.
			/// </summary>
			/// <param name="fillAmount">The number of instances to fill the pool with. If 0 is specified, the set refill amount will be used.</param>
			static void FillAllPools(int fillAmount = 0);
#pragma endregion

#pragma region Entity Allocation
			/// <summary>
			/// Returns whether the represented Entity subclass is concrete or not, that is if it can create new instances through NewInstance().
			/// </summary>
			/// <returns>Whether the represented Entity subclass is concrete or not.</returns>
			bool IsConcrete() const { return m_fpAllocate != 0 ? true : false; }

			/// <summary>
			/// Dynamically allocates an instance of the Entity subclass that this ClassInfo represents. If the Entity isn't concrete, 0 will be returned.
			/// </summary>
			/// <returns>A pointer to the dynamically allocated Entity. Ownership is transferred. If the represented Entity subclass isn't concrete, 0 will be returned.</returns>
			virtual Entity * NewInstance() const { return IsConcrete() ? m_fpNewInstance() : 0; }
#pragma endregion

		protected:

			static ClassInfo *m_sClassHead; //!< Head of unordered linked list of ClassInfos in existence.

			const std::string m_Name; //!< A string with the friendly - formatted name of this ClassInfo.
			const ClassInfo *m_pParentInfo; //!< A pointer to the parent ClassInfo.

			MemoryAllocate m_fpAllocate; //!< Raw memory allocation for the size of the type this ClassInfo describes.
			MemoryDeallocate m_fpDeallocate; //!< Raw memory deallocation for the size of the type this ClassInfo describes.
			// TODO: figure out why this doesn't want to work when defined as std::function.
			Entity *(*m_fpNewInstance)(); //!< Returns an actual new instance of the type that this describes.

			ClassInfo *m_NextClass; //!< Next ClassInfo after this one on aforementioned unordered linked list.

			std::vector<void *> m_AllocatedPool; //!< Pool of pre-allocated objects of the type described by this ClassInfo.
			int m_PoolAllocBlockCount; //!< The number of instances to fill up the pool of this type with each time it runs dry.
			int m_InstancesInUse; //!< The number of allocated instances passed out from the pool.


			// Forbidding copying
			ClassInfo(const ClassInfo &reference) {}
			ClassInfo & operator=(const ClassInfo &rhs) {}
		};
#pragma endregion

#pragma region Creation
		/// <summary>
		/// Constructor method used to instantiate a Entity in system memory. Create() should be called before using the Entity.
		/// </summary>
		Entity() { Clear(); }

		/// <summary>
		/// Makes the Entity ready for use.
		/// </summary>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Create();

		/// <summary>
		/// Creates an Entity to be identical to another, by deep copy.
		/// </summary>
		/// <param name="reference">A reference to the Entity to deep copy.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Create(const Entity &reference);

		/// <summary>
		/// Makes the Serializable ready for use.
		/// </summary>
		/// <param name="reader">A Reader that the Serializable will create itself from.</param>
		/// <param name="checkType">Whether there is a class name in the stream to check against to make sure the correct type is being read from the stream.</param>
		/// <param name="doCreate">Whether to do any additional initialization of the object after reading in all the properties from the Reader. This is done by calling Create().</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Create(Reader &reader, bool checkType = true, bool doCreate = true) { return Serializable::Create(reader, checkType, doCreate); }

		/// <summary>
		/// Uses a passed-in instance, or creates a new one, and makes it identical to this.
		/// </summary>
		/// <param name="pCloneTo">A pointer to an instance to make identical to this. If 0 is passed in, a new instance is made inside here, and ownership of it IS returned!</param>
		/// <returns>An Entity pointer to the newly cloned-to instance. Ownership IS transferred!</returns>
		virtual Entity * Clone(Entity *pCloneTo = 0) const { RTEAbort("Attempt to clone an abstract or unclonable type!"); return 0; }
#pragma endregion

#pragma region Destruction
		/// <summary>
		/// Destructor method used to clean up a Entity before deletion from system memory.
		/// </summary>
		virtual ~Entity() { Destroy(true); }

		/// <summary>
		/// Destroys and resets (through Clear()) the Entity.
		/// </summary>
		/// <param name="notInherited">Whether to only destroy the members defined in this derived class, or to destroy all inherited members also.</param>
		virtual void Destroy(bool notInherited = false) { Clear(); }

		/// <summary>
		/// Resets the entire Entity, including its inherited members, to their default settings or values.
		/// </summary>
		virtual void Reset() { Clear(); }
#pragma endregion

#pragma region INI Handling
		/// <summary>
		/// Reads a property value from a Reader stream. If the name isn't recognized by this class, then ReadProperty of the parent class is called.
		/// If the property isn't recognized by any of the base classes, false is returned, and the Reader's position is untouched.
		/// </summary>
		/// <param name="propName">The name of the property to be read.</param>
		/// <param name="reader">A Reader lined up to the value of the property to be read.</param>
		/// <returns>
		/// An error return value signaling whether the property was successfully read or not.
		/// 0 means it was read successfully, and any nonzero indicates that a property of that name could not be found in this or base classes.
		/// </returns>
		virtual int ReadProperty(std::string propName, Reader &reader);

		/// <summary>
		/// Saves the complete state of this Entity to an output stream for later recreation with Create(istream &stream).
		/// </summary>
		/// <param name="writer">A Writer that the Entity will save itself to.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Save(Writer &writer) const = 0;

		/// <summary> 
		/// Only saves out a Preset reference of this to the stream.
		/// Is only applicable to objects that are not original presets and haven't been altered since they were copied from their original.
		/// </summary>
		/// <param name="writer">A Writer that the Entity will save itself to.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int SavePresetCopy(Writer &writer) const;
#pragma endregion

#pragma region Getters and Setters
		/// <summary>
		/// Shows the ID of the DataModule this Entity has been defined in.
		/// </summary>
		/// <returns>The ID of the module, or -1 if it hasn't been defined in any.</returns>
		int GetModuleID() const { return m_DefinedInModule; }

		/// <summary>
		/// Sets the module this Entity was supposed to be defined in.
		/// </summary>
		/// <param name="whichModule">The ID of the module, or -1 if it hasn't been defined in any.</param>
		void SetModuleID(int whichModule) { m_DefinedInModule = whichModule; }

		/// <summary>
		/// Gets the name of this Entity's data Preset.
		/// </summary>
		/// <returns>A string reference with the instance name of this Entity.</returns>
		const std::string & GetPresetName() const { return m_PresetName; }

		/// <summary>
		/// Sets the name of this Entity's data Preset.
		/// </summary>
		/// <param name="newName">A string reference with the instance name of this Entity.</param>
		// TODO: Figure out how to handle if same name was set, still make it wasgivenname = true?
		virtual void SetPresetName(const std::string &newName) { /*if (m_PresetName != newName) { m_IsOriginalPreset = true; }*/ m_IsOriginalPreset = true; m_PresetName = newName; }

		/// <summary>
		/// Gets the plain text description of this Entity's data Preset.
		/// </summary>
		/// <returns>A string reference with the plain text description name of this Preset.</returns>
		const std::string & GetDescription() const { return m_PresetDescription; }

		/// <summary>
		/// Sets the plain text description of this Entity's data Preset. Shouldn't be more than a couple of sentences.
		/// </summary>
		/// <param name="newDesc">A string reference with the preset description.</param>
		void SetDescription(const std::string &newDesc) { m_PresetDescription = newDesc; }

		/// <summary>
		/// Gets the name of this Entity's data Preset, preceded by the name of the Data Module it was defined in, separated with a '/'.
		/// </summary>
		/// <returns>A string with the module and instance name of this Entity.</returns>
		std::string GetModuleAndPresetName() const;

		/// <summary>
		/// Indicates whether this Entity was explicitly given a new instance name upon creation, or if it was just copied/inherited implicitly.
		/// </summary>
		/// <returns>Whether this Entity was given a new Preset Name upon creation.</returns>
		bool IsOriginalPreset() const { return m_IsOriginalPreset; }

		/// <summary>
		/// Sets IsOriginalPreset flag to indicate that the object should be saved as CopyOf.
		/// </summary>
		void ResetOriginalPresetFlag() { m_IsOriginalPreset = false; }
#pragma endregion

#pragma region Virtual Override Methods
		/// <summary>
		/// Makes this an original Preset in a different module than it was before. It severs ties deeply to the old module it was saved in.
		/// </summary>
		/// <param name="whichModule">The ID of the new module.</param>
		/// <returns>Whether the migration was successful. If you tried to migrate to the same module it already was in, this would return false.</returns>
		virtual bool MigrateToModule(int whichModule);
#pragma endregion

#pragma region Groups
		/// <summary>
		/// Gets the list of groups this is member of.
		/// </summary>
		/// <returns>A pointer to a list of strings which describes the groups this is added to. Ownership is NOT transferred!</returns>
		const plf::list<std::string> * GetGroupList() { return &m_Groups; }

		/// <summary>
		/// Shows whether this is part of a specific group or not.
		/// </summary>
		/// <param name="whichGroup">A string which describes the group to check for.</param>
		/// <returns>Whether this Entity is in the specified group or not.</returns>
		bool IsInGroup(const std::string &whichGroup);

		/// <summary>
		/// Adds this Entity to a new grouping.
		/// </summary>
		/// <param name="newGroup">A string which describes the group to add this to. Duplicates will be ignored.</param>
		void AddToGroup(std::string newGroup) { m_Groups.push_back(newGroup); m_Groups.sort(); m_Groups.unique(); m_LastGroupSearch.clear(); }

		/// <summary>
		/// Returns random weight used in PresetMan::GetRandomBuyableOfGroupFromTech.
		/// </summary>
		/// <returns>This item's random weight from 0 to 100.</returns>
		int GetRandomWeight() const { return m_RandomWeight; }
#pragma endregion

#pragma region Lua Script Handling
		/// <summary>
		/// Reloads the Preset scripts of this Entity, from the same script file path as was originally defined.
		/// This will also update the original Preset in the PresetMan with the updated scripts so future objects spawned will use the new scripts.
		/// </summary>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int ReloadScripts() { return 0; }
#pragma endregion

#pragma region Operator Overloads
		/// <summary>
		/// A stream insertion operator for sending a Entity to an output stream.
		/// </summary>
		/// <param name="stream">An ostream reference as the left hand side operand.</param>
		/// <param name="operand">A Entity reference as the right hand side operand.</param>
		/// <returns>An ostream reference for further use in an expression.</returns>
		friend std::ostream & operator<<(std::ostream &stream, const Entity &operand) { stream << operand.GetPresetName() << ", " << operand.GetClassName(); return stream; }

		/// <summary>
		/// A Reader extraction operator for filling an Entity from a Reader.
		/// </summary>
		/// <param name="reader">A Reader reference as the left hand side operand.</param>
		/// <param name="operand">An Entity reference as the right hand side operand.</param>
		/// <returns>A Reader reference for further use in an expression.</returns>
		friend Reader & operator>>(Reader &reader, Entity &operand);

		/// <summary>
		/// A Reader extraction operator for filling an Entity from a Reader.
		/// </summary>
		/// <param name="reader">A Reader reference as the left hand side operand.</param>
		/// <param name="operand">An Entity pointer as the right hand side operand.</param>
		/// <returns>A Reader reference for further use in an expression.</returns>
		friend Reader & operator>>(Reader &reader, Entity *operand);
#pragma endregion

#pragma region Class Info
		/// <summary>
		/// Gets the ClassInfo instance of this Entity.
		/// </summary>
		/// <returns>A reference to the ClassInfo of this' class.</returns>
		virtual const Entity::ClassInfo & GetClass() const { return m_sClass; }

		/// <summary>
		/// Gets the class name of this Entity.
		/// </summary>
		/// <returns>A string with the friendly-formatted type name of this Entity.</returns>
		virtual const std::string & GetClassName() const { return m_sClass.GetName(); }
#pragma endregion

	protected:

		static Entity::ClassInfo m_sClass; //!< Type description of this Entity.

		std::string m_PresetName; //!< The name of the Preset data this was cloned from, if any.
		std::string m_PresetDescription; //!< The description of the preset in user friendly plain text that will show up in menus etc.

		bool m_IsOriginalPreset; //!< Whether this is to be added to the PresetMan as an original preset instance.  
		int m_DefinedInModule; //!< The DataModule ID that this was successfully added to at some point. -1 if not added to anything yet.

		//TODO Consider replacing this with an unordered_set. See https://github.com/cortex-command-community/Cortex-Command-Community-Project-Source/issues/88
		plf::list<std::string> m_Groups; //!< List of all tags associated with this. The groups are used to categorize and organize Entities.  
		std::string m_LastGroupSearch; //!< Last group search string, for more efficient response on multiple tries for the same group name.  
		bool m_LastGroupResult; //!< Last group search result, for more efficient response on multiple tries for the same group name.

		int m_RandomWeight; //!< Random weight used when picking item using PresetMan::GetRandomBuyableOfGroupFromTech. From 0 to 100. 0 means item won't be ever picked.

		// Forbidding copying
		Entity(const Entity &reference) {}
		Entity & operator=(const Entity &rhs) { return *this; }

	private:

		/// <summary>
		/// Clears all the member variables of this Entity, effectively resetting the members of this abstraction level only.
		/// </summary>
		void Clear();
	};
}
#endif