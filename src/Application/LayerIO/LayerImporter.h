/*
 For more information, please see: http://software.sci.utah.edu

 The MIT License

 Copyright (c) 2009 Scientific Computing and Imaging Institute,
 University of Utah.


 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 */

#ifndef APPLICATION_LAYERIO_LAYERIMPORTER_H
#define APPLICATION_LAYERIO_LAYERIMPORTER_H

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif 

// STL includes
#include <vector>
#include <string>
#include <map>

// Boost includes
#include <boost/filesystem.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

// Core includes
#include <Core/DataBlock/DataType.h>
#include <Core/Utils/EnumClass.h>
#include <Core/Utils/Log.h>

// Application includes
#include <Application/Layer/Layer.h>

namespace Seg3D
{

// ------------------------------------------------------
// CLASS LayerImporterMode:
// The modes that define how data can be imported

// LayerImporterMode class definition
CORE_ENUM_CLASS
(
	LayerImporterMode,
	// -- UNKNOWN --
	// Invalid importer mode
	INVALID_E = 0x0000,

	// -- DATA --
	// Convert the data into a data volume
	DATA_E = 0x0001,

	// -- SINGLE_MASK --
	// Convert the data such that every value that is not zero is assumed to indicate the
	// mask.
	SINGLE_MASK_E = 0x0002,
	
	// -- BIT_MASKS --
	// Every bitplane that has a bit set will be assumed to be a separate mask
	BITPLANE_MASK_E   = 0x0004,

	// -- LABEL_MASKS --
	// Each separate number is interpreted as a separate mask
	LABEL_MASK_E = 0x0008	
)


std::string ExportToString( LayerImporterMode mode );
bool ImportFromString( const std::string& import_type_string, LayerImporterMode& mode );

// ------------------------------------------------------
// CLASS LayerImporter:
// The LayerImporter class abstracts the process of importing a layer

// This is a base class from which importers are sub classed
// The following functions need to be implemented:

// check_header: Fast function that checks whether importer can import file, this function is
// intended to help with auto selection of which importer to use. If the importer deals with all
// the file types that it has registered, this function does not need to be reimplemented.

// import_header: This function is called before querying any of the header information. This 
// function is called on a separate thread and hence it should do the time consuming pieces. If no
// separate header reading is supported, the importer should read the full file at this time.

// get_grid_transform: Get the transform of the dataset for the information display screen
// get_data_type: Get the data type for the information display screen




// forward declaration
class LayerImporter;
typedef boost::shared_ptr< LayerImporter > LayerImporterHandle;
typedef boost::weak_ptr< LayerImporter > LayerImporterWeakHandle;

class LayerImporterPrivate;
typedef boost::shared_ptr< LayerImporterPrivate > LayerImporterPrivateHandle;

// class definition
class LayerImporter : public boost::noncopyable
{
	// -- Constructor/Destructor --
public:
	// Construct a new layer file importer for a specific file
	LayerImporter( const std::string& filename );

	// Virtual destructor for memory management of derived classes
	virtual ~LayerImporter();

	// These functions define the properties of the importer and are implemented
	// by the SCI_IMPORTER_TYPE macro

	// -- Get the properties of this importer --
	// These properties are generated by the SCI_IMPORTER_TYPE macro
public:
	// Name of the importer (to be listed in the file filter dialog)
	virtual std::string name() const = 0;
	// The file types (extentions) that the importer handles
	virtual std::string file_types() const = 0;
	// Priority flag that resolves which importer needs to be used if multiple are available for
	// a similar file type
	virtual unsigned int priority() const = 0;

	// -- Filename handling --
public:
	// GET_FILENAME:
	// Get the filename that this importer is importing
	std::string get_filename();
	
	// GET_BASE_FILENAME
	// Get the filename without path and without extension
	std::string get_base_filename();
	
	// -- Error handling --
	// GET_ERROR:
	// Get the last error recorded in the importer
	std::string get_error() const;
	
protected:
	// SET_ERROR:
	// Set the error message
	void set_error( const std::string& error );
	
	// -- Import a header information --
public:
	// CHECK_HEADER:
	// A function that makes sure that the current importer can handle the file. In most cases
	// this function does not need to be overloaded.
	virtual bool check_header();

	// IMPORT_HEADER:
	// Import all the information needed to translate the header and metadata information, but not
	// necessarily read the whole file. NOTE: Some external packages do not support reading a header
	// and hence these importers should read the full file here.
	virtual bool import_header() = 0;
		
	// GET_GRID_TRANSFORM:
	// Get the grid transform of the grid that we are importing
	virtual Core::GridTransform get_grid_transform() = 0;

	// GET_DATA_TYPE:
	// Get the type of data that is being imported
	virtual Core::DataType get_data_type() = 0;
	
	// GET_IMPORTER_MODES
	// Get then supported importer modes
	virtual int get_importer_modes() = 0;

	// SET_FILE_LIST:
	// In the case where we are importing a series, this sets the list of files for the importer
	// to import.
	virtual bool set_file_list( const std::vector< std::string >& file_list ){ return false; }

	// -- Import the data as a specific type --	
public:	
	// IMPORT_LAYER
	// Import the layer from the file
	bool import_layer( LayerImporterMode mode, std::vector< LayerHandle >& layers );

	// SET_SWAP_XY_SPACING:
	// Set whether to swap the X/Y spacing.
	// NOTE: This only affects the behavior when loading DICOM files.
	// The DICOM standard says that pixel spacing is in Y/X order, 
	// but some scanners put it in X/Y order. 
	void set_swap_xy_spacing( bool swap );

	// GET_SWAP_XY_SPACING:
	// Get whether to swap the X/Y spacing.
	bool get_swap_xy_spacing() const;

protected:
	// LOAD_DATA:
	// Load the data from the file(s).
	// NOTE: This function is called by import_layer internally.
	virtual bool load_data( Core::DataBlockHandle& data_block, 
		Core::GridTransform& grid_trans ) = 0;

	// GET_LAYER_NAME:
	// Return the string that will be used to name the layers.
	virtual std::string get_layer_name();
	
	// -- Internals of the importer --
private:
	LayerImporterPrivateHandle private_;
};


// SCI_IMPORTER_TYPE:
// This macro should be defined at the top of each LayerImporter. It renders 
// code that allows both the class as well as the object to determine what its 
// properties are. By defining class specific static functions the class 
// properties can be queried without instantiating the object. On the other
// hand you want to query these properties from the object as well, even when
// we only have a pointer to the base object. Hence we need virtual functions
// as well. 

// Note: one would expect to use virtual static functions, but those are not
// allowed in C++, hence this macro ensures a consistent definition.

#define SCI_IMPORTER_TYPE(importer_name,importer_file_types,importer_priority) \
  public: \
    static std::string  Name()       { return importer_name; }\
    static std::string  FileTypes()  { return importer_file_types; }\
    static unsigned int Priority()   { return importer_priority; }\
    \
    virtual std::string  name() const       { return Name(); } \
    virtual std::string  file_types() const { return FileTypes(); } \
    virtual unsigned int priority() const   { return Priority(); }

} // end namespace seg3D

#endif
