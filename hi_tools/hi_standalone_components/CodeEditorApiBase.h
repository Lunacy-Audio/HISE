/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;


struct ValueTreeApiHelpers
{
	static AttributedString createAttributedStringFromApi(const ValueTree &method, const String &, bool multiLine, Colour textColour);

	static String createCodeToInsert(const ValueTree &method, const String &className);

	static void getColourAndCharForType(int type, char &c, Colour &colour);



};

class DebugInformationBase;

class DebugableObjectBase
{
public:

	struct Location
	{
		String fileName = String();
		int charNumber = 0;
	};

	virtual ~DebugableObjectBase() {};

	/** Override this and return the class id of this object. */
	virtual Identifier getObjectName() const = 0;

	/** This will be shown as value of the object. */
	virtual String getDebugValue() const { return ""; };

	/** This will be shown as name of the object. */
	virtual String getDebugName() const { return getInstanceName().toString(); };

	virtual String getDebugDataType() const { return getDebugName(); }

	virtual AttributedString getDescription() const
	{
		return AttributedString();
	}

	virtual String getCategory() const { return ""; }

	/** Override this if you don't want this object to show up in the list. */
	virtual bool isInternalObject() const { return false; }

	virtual int getTypeNumber() const { return 0; }

	

	virtual Identifier getInstanceName() const { return getObjectName(); }

	/** This can be used to override the debug information created for one of its objects children (constant or function). If you do so, the object created here will be owned by the caller. */
	virtual DebugInformationBase* createDebugInformationForChild(const Identifier& id)
	{
		return nullptr;
	}

	virtual void getAllFunctionNames(Array<Identifier>& functions) const {};

	virtual void getAllConstants(Array<Identifier>& ids) const {};

	virtual const var getConstantValue(int index) const { return var(); };

	/** This will be called if the user double clicks on the row. */
	virtual void doubleClickCallback(const MouseEvent &/*e*/, Component* /*componentToNotify*/) {};

	virtual void rightClickCallback(const MouseEvent& /*e*/, Component* /*componentToNotifiy*/) {};

	virtual Location getLocation() const { return Location(); }

	JUCE_DECLARE_WEAK_REFERENCEABLE(DebugableObjectBase);
};



class DynamicDebugableObjectWrapper : public DebugableObjectBase
{
public:

	DynamicDebugableObjectWrapper(DynamicObject::Ptr obj_, const Identifier& className_, const Identifier& instanceId_) :
		obj(obj_),
		className(className_),
		instanceId(instanceId_)
	{

	}

	virtual Identifier getObjectName() const override { return className; }
	virtual Identifier getInstanceName() const override { return instanceId; }

	virtual String getDebugValue() const override { return getInstanceName().toString(); }

	virtual void getAllFunctionNames(Array<Identifier>& functions) const
	{
		for (const auto& p : obj->getProperties())
		{
			if (p.value.isMethod())
				functions.add(p.name);
		}
	};

	virtual void getAllConstants(Array<Identifier>& ids) const
	{
		for (const auto& p : obj->getProperties())
		{
			if (p.value.isMethod())
				continue;

			ids.add(p.name);
		}
	};

	

	virtual const var getConstantValue(int index) const { return obj->getProperties().getValueAt(index); };

	Identifier className;
	Identifier instanceId;
	DynamicObject::Ptr obj;
};


/** A general representation of an entity that is supposed to be shown in the IDE. */
class DebugInformationBase
{
public:

	/** This will be called if the user double clicks on the row. */
	virtual void doubleClickCallback(const MouseEvent &e, Component* componentToNotify)
	{
		if (auto obj = getObject())
			getObject()->doubleClickCallback(e, componentToNotify);
	};

	virtual void rightClickCallback(const MouseEvent& e, Component* componentToNotify)
	{
		if (auto obj = getObject())
			getObject()->rightClickCallback(e, componentToNotify);
	};

	virtual int getType() const 
	{ 
		if (auto obj = getObject())
			return obj->getTypeNumber();

		return 0; 
	};

	virtual String getTextForName() const
	{
		if (auto obj = getObject())
			return obj->getDebugName();

		return "undefined";
	}

	virtual String getCategory() const
	{ 
		if (auto obj = getObject())
			return obj->getCategory();

		return ""; 
	}

	virtual DebugableObjectBase::Location getLocation() const
	{
		if (auto obj = getObject())
			return obj->getLocation();

		return DebugableObjectBase::Location();
	}

	virtual String getTextForType() const { return "unknown"; }

	virtual String getTextForDataType() const 
	{ 
		if (auto obj = getObject()) 
			return obj->getDebugDataType(); 

		return "undefined";
	}

	virtual String getTextForValue() const
	{
		if (auto obj = getObject())
			return obj->getDebugValue();

		return "empty";
	}

	virtual String getCodeToInsert() const = 0;

	virtual AttributedString getDescription() const
	{
		if (auto obj = getObject())
			return obj->getDescription();

		return AttributedString();
	}

	virtual DebugableObjectBase* getObject() { return nullptr; }
	virtual const DebugableObjectBase* getObject() const { return nullptr; }

	virtual ~DebugInformationBase() {};

	static String getVarType(const var &v)
	{
		if (v.isUndefined())	return "undefined";
		else if (v.isArray())	return "Array";
		else if (v.isBool())	return "bool";
		else if (v.isInt() ||
			v.isInt64())	return "int";
		else if (v.isObject())
		{
			if (auto d = dynamic_cast<DebugableObjectBase*>(v.getObject()))
			{
				return d->getDebugDataType();
			}
			else return "Object";
		}
		else if (v.isObject()) return "Object";
		else if (v.isDouble()) return "double";
		else if (v.isString()) return "String";
		else if (v.isMethod()) return "function";

		return "undefined";
	}

	StringArray createTextArray() const
	{
		StringArray sa;

		sa.add(getTextForType());
		sa.add(getTextForDataType());
		sa.add(getTextForName());
		sa.add(getTextForValue());

		return sa;
	}
};


class SettableDebugInfo : public DebugInformationBase
{
public:

	SettableDebugInfo()
	{};

	int typeValue = 0;
	String name;
	String dataType;
	String type;
	String value;
	String codeToInsert;
	String category;
	AttributedString description;

	int getType() const { return typeValue; };
	String getTextForName() const { return name; }
	String getTextForType() const { return type; }
	String getTextForDataType() const { return dataType; }
	String getTextForValue() const { return value; }
	String getCodeToInsert() const { return codeToInsert; }
	String getCategory() const { return category; }

	AttributedString getDescription() const { return description; }
};



class ObjectDebugInformation : public DebugInformationBase
{
public:

	ObjectDebugInformation(DebugableObjectBase* b, int type_=-1) :
		obj(b),
		type(type_)
	{

	}

	int getType() const override { return type >= 0 ? type : obj->getTypeNumber(); }

	int type;

	virtual String getCodeToInsert() const override { return getObject()->getInstanceName().toString(); };
	DebugableObjectBase* getObject() override { return obj.get(); }
	const DebugableObjectBase* getObject() const override { return obj.get(); }

	WeakReference<DebugableObjectBase> obj;
};


struct ManualDebugObject : public DebugableObjectBase
{
	juce::String getDebugValue() const override { return ""; }
	Identifier getObjectName() const override { return objectName; }
	juce::String getCategory() const override { return category; }

	template <class T> static DebugInformationBase* create()
	{
		return new Holder(new T());
	}

	void getAllConstants(Array<Identifier>& ids) const override
	{
		ids.addArray(childNames);
	}

	DebugInformationBase* createDebugInformationForChild(const Identifier& id) override
	{
		if (childNames.contains(id))
		{
			auto i = new SettableDebugInfo();
			i->category = category;
			fillInfo(i, id.toString());

			return i;
		}

		return nullptr;
	}

protected:

	Identifier objectName;
	Array<Identifier> childNames;
	juce::String category;

	virtual void fillInfo(SettableDebugInfo* childInfo, const juce::String& id) = 0;

private:

	struct Holder : public ObjectDebugInformation
	{
		Holder(DebugableObjectBase* ownedObject) :
			ObjectDebugInformation(nullptr)
		{
			dummy = ownedObject;
			obj = dummy;
		}

		DebugableObjectBase* getObject() override
		{
			return dummy.get();
		}

		const DebugableObjectBase* getObject() const override
		{
			return dummy.get();
		}

		ScopedPointer<DebugableObjectBase> dummy;
	};
};

class JavascriptCodeEditor;

class ApiProviderBase
{
public:

	class ApiComponentBase;

	/** This object is supposed to manage an ApiProviderBase object.

		The lifetime of an ApiProviderBase object might be very short, so this makes sure that
		the getProviderBase() method always returns the most up to date object. */
	struct Holder
	{
		virtual ~Holder() {};

		/** Override this method and return a provider if it exists. */
		virtual ApiProviderBase* getProviderBase() = 0;

		void addEditor(Component* editor)
		{
			repaintUpdater.editors.add(editor);
		}

		void removeEditor(Component* editor)
		{
			repaintUpdater.editors.removeAllInstancesOf(editor);
		}

		virtual int getCodeFontSize() const { return 15; }

		virtual void setActiveEditor(JavascriptCodeEditor* e, CodeDocument::Position pos) {}

		virtual JavascriptCodeEditor* getActiveEditor() { return nullptr; }

		/** Override this method and return a ValueTree containing the API documentation. */
		virtual ValueTree createApiTree() { return {}; };

		virtual void jumpToDefinition(const String& token, const String& namespaceId) {
			ignoreUnused(token, namespaceId);
		}

		virtual void rebuild();;

		virtual bool handleKeyPress(const KeyPress& k, Component* c) { return false; }

		virtual void addPopupMenuItems(PopupMenu &m, Component* c, const MouseEvent& e) {};
		virtual void performPopupMenuAction(int menuId, Component* c) {};

		virtual void handleBreakpoints(const Identifier& codeFile, Graphics& g, Component* c) {};

		virtual void handleBreakpointClick(const Identifier& codeFile, CodeEditorComponent& ed, const MouseEvent& e)
		{
		}

	protected:

		struct RepaintUpdater : public AsyncUpdater
		{
			void handleAsyncUpdate() override
			{
				for (int i = 0; i < editors.size(); i++)
				{
					editors[i]->repaint();
				}
			}

			Array<Component::SafePointer<Component>> editors;
		};

		friend class ApiComponentBase;

		Array<WeakReference<ApiComponentBase>> registeredComponents;

		RepaintUpdater repaintUpdater;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
	};

	/** Subclass any component from this interface class and use getProviderBase(). Check for nullptr each time!. */
	class ApiComponentBase
	{
	public:
		ApiProviderBase* getProviderBase()
		{
			if (holder != nullptr)
				return holder->getProviderBase();

			return nullptr;
		}

		virtual void providerWasRebuilt() {};

	protected:

		void registerAtHolder()
		{
			if (holder != nullptr)
				holder->registeredComponents.addIfNotAlreadyThere(this);
		}

		void deregisterAtHolder()
		{
			if (holder != nullptr)
				holder->registeredComponents.removeAllInstancesOf(this);
		}

		ApiComponentBase(Holder* h) :
			holder(h)
		{
			registerAtHolder();
		};

		virtual ~ApiComponentBase()
		{
			deregisterAtHolder();
		}

		WeakReference<Holder> holder;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ApiComponentBase);
	};

	virtual ~ApiProviderBase() {};

	/** Override this method and return the number of all objects that you want to use. */
	virtual int getNumDebugObjects() const = 0;

	virtual DebugInformationBase* getDebugInformation(int index) = 0;

	/** Override this method and return a Colour and a (uppercase) letter to be displayed in the autocomplete window for each type. */
	virtual void getColourAndLetterForType(int type, Colour& colour, char& letter);

	/** Override this and return a string that will be displayed when you hover over a token. */
	virtual String getHoverString(const String& token) 
	{ 
		if (auto obj = getDebugObject(token))
		{
			String s;

			s << obj->getDebugDataType() << " " << obj->getDebugName() << ": " << obj->getDebugValue();
			return s;
		}

		return "";
	}

	/** Override this method and return the object for the given token.

		The default implementation checks the name and instance id of each registered debug object, but you can overload it with more complex functions.
	*/
	virtual DebugableObjectBase* getDebugObject(const String& token);
};

} // namespace hise


