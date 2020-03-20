#ifndef _RTEDATAMODULE_
#define _RTEDATAMODULE_

#include "ContentFile.h"
#include "Constants.h"

//struct DATAFILE; // DataFile loading not implemented.
struct BITMAP;

namespace RTE {

	class Entity;

	/// <summary>
	/// A representation of a DataModule containing zero or many Material, Effect, Ammo, Device, Actor, or Scene definitions.
	/// </summary>
	class DataModule : public Serializable {
		friend class LuaMan;

		/// <summary>
		/// Holds and owns the actual object instance pointer, and the location of the data file it was read from, as well as where in that file.
		/// </summary>
		struct PresetEntry {
			Entity *m_pEntityPreset; //!< Owned by this.
			std::string m_FileReadFrom; //!< Where the instance was read from.
			PresetEntry(Entity *pPreset, std::string file) { m_pEntityPreset = pPreset; m_FileReadFrom = file; }
		};

	public:

#pragma region Creation
		/// <summary>
		/// Constructor method used to instantiate a DataModule object in system memory. Create() should be called before using the object.
		/// </summary>
		DataModule() { Clear(); }

		/// <summary>
		/// Constructor method used to instantiate a DataModule object in system memory, and also do a Create() in the same line.
		/// Create() should therefore not be called after using this constructor.
		/// </summary>
		/// <param name="moduleName">A string defining the path to where the content file itself is located, either within the package file, or directly on the disk.</param>
		/// <param name="fpProgressCallback">A function pointer to a function that will be called and sent a string with information about the progress of this DataModule's creation.</param>
		DataModule(std::string moduleName, ProgressCallback fpProgressCallback = 0) { Clear(); Create(moduleName, fpProgressCallback); }

		/// <summary>
		/// Makes the DataModule object ready for use. This needs to be called after PresetMan is created.
		/// This looks for an "index.ini" within the specified .rte directory and loads all the defined objects in that index file. 
		/// </summary>
		/// <param name="moduleName">A string defining the name of this DataModule, e.g. "MyModule.rte".</param>
		/// <param name="fpProgressCallback">A function pointer to a function that will be called and sent a string with information about the progress of this DataModule's creation.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Create(std::string moduleName, ProgressCallback fpProgressCallback = 0);
#pragma endregion

#pragma region Destruction
		/// <summary>
		/// Destructor method used to clean up a DataModule object before deletion from system memory.
		/// </summary>
		virtual ~DataModule() { Destroy(true); }

		/// <summary>
		/// Destroys and resets (through Clear()) the DataModule object.
		/// </summary>
		/// <param name="notInherited">Whether to only destroy the members defined in this derived class, or to destroy all inherited members also.</param>
		virtual void Destroy(bool notInherited = false);

		/// <summary>
		/// Resets the entire DataModule, including its inherited members, to their default settings or values.
		/// </summary>
		virtual void Reset() { Clear(); }
#pragma endregion

#pragma region INI Handling
		/// <summary>
		/// Read module specific properties from index.ini without processing IncludeFiles and loading the whole module.
		/// </summary>
		/// <param name="moduleName">A string defining the name of this DataModule, e.g. "MyModule.rte".</param>
		/// <param name="fpProgressCallback">A function pointer to a function that will be called and sent a string with information about the progress of this DataModule's creation.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int ReadModuleProperties(std::string moduleName, ProgressCallback fpProgressCallback = 0);

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
		/// Saves the complete state of this DataModule to an output stream for later recreation with Create(Reader &reader).
		/// </summary>
		/// <param name="writer">A Writer that the DataModule will save itself with.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		virtual int Save(Writer &writer) const;

		/// <summary>
		/// Returns true if loader should ignore missing items in this module.
		/// </summary>
		/// <returns>True if loader should ignore missing items in this module.</returns>
		const bool GetIgnoreMissingItems() const { return m_IgnoreMissingItems; }
#pragma endregion

#pragma region Module Information Getters
		/// <summary>
		/// Gets the file name of this DataModule, e.g. "MyMod.rte".
		/// </summary>
		/// <returns>A string with the data module file name.</returns>
		virtual const std::string & GetFileName() const { return m_FileName; }

		/// <summary>
		/// Gets the friendly name of this DataModule, e.g. "My Great Mod".
		/// </summary>
		/// <returns>A string with the data module's friendly name.</returns>
		virtual const std::string & GetFriendlyName() const { return m_FriendlyName; }

		/// <summary>
		/// Gets the author name of this DataModule, e.g. "Data Realms, LLC".
		/// </summary>
		/// <returns>A string with the author's name.</returns>
		virtual const std::string & GetAuthor() const { return m_Author; }

		/// <summary>
		/// Gets the description of this DataModule's contents.
		/// </summary>
		/// <returns>A string with the description.</returns>
		virtual const std::string & GetDescription() const { return m_Description; }

		/// <summary>
		/// Gets the version number of this DataModule.
		/// </summary>
		/// <returns>An int with the version number, starting at 1.</returns>
		virtual int GetVersionNumber() const { return m_Version; }

		/// <summary>
		/// Gets the BITMAP that visually represents this DataModule, for use in menus.
		/// </summary>
		/// <returns>BITMAP pointer that might have the icon. 0 is very possible.</returns>
		BITMAP * GetIcon() const { return m_pIcon; }

		/// <summary>
		/// Returns crab-to-human spawn ration for this tech.
		/// </summary>
		/// <returns>Crab-to-human spawn ration value.</returns>
		const float GetCrabToHumanSpawnRatio() const { return m_CrabToHumanSpawnRatio; }
#pragma endregion

#pragma region Entity Mapping
		/// <summary>
		/// Gets the data file path of a previously read in (defined) Entity.
		/// </summary>
		/// <param name="exactType">The type name of the derived Entity. Ownership is NOT transferred!</param>
		/// <param name="instance">The instance name of the derived Entity instance.</param>
		/// <returns>The file path of the data file that the specified Entity was read from. If no Entity of that description was found, "" is returned.</returns>
		std::string GetEntityDataLocation(std::string exactType, std::string instance);

		/// <summary>
		/// Gets a previously read in (defined) Entity, by exact type and instance name. Ownership is NOT transferred!
		/// </summary>
		/// <param name="exactType">The exact type name of the derived Entity instance to get.</param>
		/// <param name="instance">The instance name of the derived Entity instance.</param>
		/// <returns>A pointer to the requested Entity instance. 0 if no Entity with that derived type or instance name was found. Ownership is NOT transferred!</returns>
		const Entity * GetEntityPreset(std::string exactType, std::string instance);

		/// <summary>
		/// Adds an Entity instance's pointer and name associations to the internal list of already read in Entities. Ownership is NOT transferred!
		/// If there already is an instance defined, nothing happens. If there is not, a clone is made of the passed-in Entity and added to the library.
		/// </summary>
		/// <param name="pEntToAdd">A pointer to the Entity derived instance to add. It should be created from a Reader. Ownership is NOT transferred!</param>
		/// <param name="overwriteSame">
		/// Whether to overwrite if an instance of the EXACT same TYPE and name was found.
		/// If one of the same name but not the exact type, false is returned regardless and nothing will have been added.
		/// </param>
		/// <param name="readFromFile">
		/// The file the instance was read from, or where it should be written.
		/// If "Same" is passed as the file path read from, an overwritten instance will keep the old one's file location entry.
		/// </param>
		/// <returns>
		/// Whether or not a copy of the passed-in instance was successfully inserted into the module.
		/// False will be returned if there already was an instance of that class and instance name inserted previously, unless overwritten.
		/// </returns>
		bool AddEntityPreset(Entity *pEntToAdd, bool overwriteSame = false, std::string readFromFile = "Same");

		/// <summary>
		/// Gets the list of all registered Entity groups of this.
		/// </summary>
		/// <returns>The list of all groups. Ownership is not transferred.</returns>
		const plf::list<std::string> * GetGroupRegister() const { return &m_GroupRegister; }

		/// <summary>
		/// Registers the existence of an Entity group in this module.
		/// </summary>
		/// <param name="newGroup">The group to register.</param>
		void RegisterGroup(std::string newGroup) { m_GroupRegister.push_back(newGroup); m_GroupRegister.sort(); m_GroupRegister.unique(); }

		/// <summary>
		/// Fills out a list with all groups registered with this that contain any objects of a specific type and it derivatives.
		/// </summary>
		/// <param name="groupList">The list that all found groups will be ADDED to. OINT.</param>
		/// <param name="withType">The name of the type to only get groups of.</param>
		/// <returns>Whether any groups with the specified type was found.</returns>
		bool GetGroupsWithType(plf::list<std::string> &groupList, std::string withType);

		/// <summary>
		/// Adds to a list all previously read in (defined) Entities which are associated with a specific group.
		/// </summary>
		/// <param name="objectList">Reference to a list which will get all matching Entities added to it. Ownership of the list or the Entities placed in it are NOT transferred!</param>
		/// <param name="group">The group to look for.</param>
		/// <param name="type">The name of the least common denominator type of the Entities you want. "All" will look at all types.</param>
		/// <returns>Whether any Entity:s were found and added to the list.</returns>
		bool GetAllOfGroup(plf::list<Entity *> &objectList, std::string group, std::string type);

		/// <summary>
		/// Adds to a list all previously read in (defined) Entities, by inexact type.
		/// </summary>
		/// <param name="objectList">Reference to a list which will get all matching Entities added to it. Ownership of the list or the Entities placed in it are NOT transferred!</param>
		/// <param name="type">The name of the least common denominator type of the Entities you want. "All" will look at all types.</param>
		/// <returns>Whether any Entities were found and added to the list.</returns>
		bool GetAllOfType(plf::list<Entity *> &objectList, std::string type);
#pragma endregion

#pragma region Material Mapping
		/// <summary>
		/// Gets a Material mapping local to this DataModule.
		/// This is used for when multiple DataModules are loading conflicting Materials, and need to resolve the conflicts by mapping their materials to ID's different than those specified in the data files.
		/// </summary>
		/// <param name="materialID">The material ID to get the mapping for.</param>
		/// <returns>The material ID that the passed in ID is mapped to, if any. 0 if no mapping present.</returns>
		int GetMaterialMapping(int materialID) const { return m_MaterialMappings[materialID]; }

		/// <summary>
		/// Gets the entire Material mapping array local to this DataModule.
		/// </summary>
		/// <returns>A pointer to the entire local mapping array, 256 unsigned chars. Ownership is NOT transferred!</returns>
		const unsigned char * GetAllMaterialMappings() const { return m_MaterialMappings; }

		/// <summary>
		/// Adds a Material mapping local to a DataModule.
		/// This is used for when multiple DataModules are loading conflicting Materials, and need to resolve the conflicts by mapping their materials to ID's different than those specified in the data files.
		/// </summary>
		/// <param name="fromID">The material ID to map from.</param>
		/// <param name="toID">The material ID to map to.</param>
		/// <returns>Whether this created a new mapping which didn't override a previous material mapping.</returns>
		bool AddMaterialMapping(int fromID, int toID);
#pragma endregion

#pragma region Lua Script Handling
		/// <summary>
		/// Loads the preset scripts of this object, from a specified path.
		/// </summary>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		int LoadScripts();

		/// <summary>
		/// Reloads all scripted Entity Presets with the latest version of their respective script files.
		/// </summary>
		void ReloadAllScripts();
#pragma endregion

#pragma region Class Info
		/// <summary>
		/// Gets the class name of this Entity.
		/// </summary>
		/// <returns>A string with the friendly-formatted type name of this object.</returns>
		virtual const std::string & GetClassName() const { return m_ClassName; }
#pragma endregion

	protected:

		static const std::string m_ClassName; //!< A string with the friendly-formatted type name of this object.

		bool m_ScanFolderContents; //!< Indicates whether module loader should scan for any .ini's inside module folder instead of loading files defined in IncludeFile only.
		bool m_IgnoreMissingItems; //!< Indicates whether module loader should ignore missing items in this module.

		std::string m_FileName; //!< File/folder name of the data module, eg "MyMod.rte".   
		std::string m_FriendlyName; //!< Friendly name of the data module, eg "My Weapons Mod". 
		std::string m_Author; //!< Name of the author of this module.
		std::string m_Description; //!< Brief description of what this module is and contains.
		std::string m_ScriptPath; //!< Path to script to execute when this module is loaded.
		int m_Version; //!< Version number, starting with 1.
		int m_ModuleID; //!< ID number assigned to this upon loading, for internal use only, don't reflect in ini's.

		ContentFile m_IconFile; //!< File to the icon/symbol bitmap.
		BITMAP *m_pIcon; //!< Bitmap with the icon loaded form above file.

		float m_CrabToHumanSpawnRatio; //!< Crab-to-human Spawn ratio to replace value from Constants.lua.

		plf::list<const Entity *> m_EntityList; //!< A list of loaded entities solely for the purpose of enumeration presets from Lua.
		plf::list<std::string> m_GroupRegister; //!< List of all Entity groups ever registered in this, all uniques.
		unsigned char m_MaterialMappings[c_PaletteEntriesNumber]; //!< Material mappings local to this DataModule.

		/// <summary>
		/// Ordered list of all owned Entity instances, ordered by the sequence of their reading - really now with overwriting?.
		/// This is used to be able to write back all of them in proper order into their respective files in the DataModule when writing this.
		/// The Entity instances ARE owned by this list.
		/// </summary>
		plf::list<PresetEntry> m_PresetList;

		/// <summary>
		/// Map of class names and map of instance template names and actual Entity instances that were read for this DataModule.
		/// An Entity instance of a derived type will be placed in EACH of EVERY of its parent class' maps here.
		/// There can be multiple entries of the same instance name in any of the type sub-maps, but only ONE whose exact class is that of the type-list!
		/// The Entity instances are NOT owned by this map.
		/// </summary>
		std::map<std::string, plf::list<std::pair<std::string, Entity *>>> m_TypeMap;

#pragma region Entity Mapping
		/// <summary>
		/// Checks if the type map has an instance added of a specific name and exact type.
		/// Does not check if any parent types with that name has been added. If found, that instance is returned, otherwise 0.
		/// </summary>
		/// <param name="exactType">The exact type name to look for.</param>
		/// <param name="instanceName">The exact instance name to look for.</param>
		/// <returns>The found Entity Preset of the exact type and name, if found.</returns>
		Entity * GetEntityIfExactType(const std::string &exactType, const std::string &instanceName);

		/// <summary>
		/// Adds a newly added preset instance to the type map, where it will end up in every type-list of every class it derived from as well.
		/// I.e the "Entity" map will contain every single instance added to this.
		/// This will NOT check if duplicates are added to any type-list, so please use GetEntityIfExactType to check this beforehand.
		/// Dupes are allowed if there are no more than one of the exact class and name.
		/// </summary>
		/// <param name="pEntToAdd">The new object instance to add. OINT!</param>
		/// <returns>Whether the Entity was added successfully or not.</returns>
		bool AddToTypeMap(Entity *pEntToAdd);
#pragma endregion

	private:

		/// <summary>
		/// Creates a DataModule to be identical to another, by deep copy. Private method to prevent from cloning Data Modules.
		/// </summary>
		/// <param name="reference">A reference to the DataModule to deep copy.</param>
		/// <returns>An error return value signaling success or any particular failure. Anything below 0 is an error signal.</returns>
		int Create(const DataModule &reference);

		/// <summary>
		/// Clears all the member variables of this DataModule, effectively resetting the members of this abstraction level only.
		/// </summary>
		void Clear();
	};
}
#endif