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

#ifndef FILEBROWSER_H_INCLUDED
#define FILEBROWSER_H_INCLUDED

namespace hise { using namespace juce;

class FileBrowser;

class FileBrowserToolbarFactory : public ToolbarItemFactory
{
public:

	FileBrowserToolbarFactory(FileBrowser *browser_) :
		browser(browser_)
	{

	};

	void getAllToolbarItemIds(Array<int> &ids) override;

	void getDefaultItemSet(Array<int> &ids) override { getAllToolbarItemIds(ids); };

	ToolbarItemComponent * createItem(int itemId);

	struct FileBrowserToolbarPaths
	{
		static Drawable *createPath(int id, bool isOn);
	};

private:

	FileBrowser *browser;

	
};


class BackendProcessorEditor;

class FileBrowser : public Component,
					public DragAndDropContainer,
					public ApplicationCommandTarget,
					public TextEditor::Listener,
					public ProjectHandler::Listener,
					public ExpansionHandler::Listener
{
public:

	struct Favorite : public RestorableObject
	{
		Favorite(const String &name_, const File &directory_) :
			name(name_),
			directory(directory_)
		{};

		String name;
		File directory;

		void restoreFromValueTree(const ValueTree &previouslyExportedState)
		{
			name = previouslyExportedState.getProperty("Name");
			directory = File(previouslyExportedState.getProperty("File").toString());
		}

		ValueTree exportAsValueTree() const override
		{
			ValueTree v("Favorite");
			
			v.setProperty("Name", name, nullptr);
			v.setProperty("File", directory.getFullPathName(), nullptr);

			return v;
		}

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Favorite);
	};

	/** All application commands are collected here. */
	enum FileBrowserToolbarCommands
	{
		ShowFavoritePopup = 0x70000,
		AddFavorite,
		RemoveFavorite,
		HardDisks,
		Back,
		Forward,
		numFileBrowserCommands
	};

	FileBrowser(BackendRootWindow* rootWindow);

	SET_GENERIC_PANEL_ID("FileBrowser");

	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	void projectChanged(const File& newRootDirectory) override;

	void expansionPackCreated(Expansion* newExpansion) override
	{
		expansionPackLoaded(newExpansion);
	}

	void expansionPackLoaded(Expansion* currentExpansion) override;

	void goToDirectory(const File &newRoot, bool useUndoManager=true);

	void getAllCommands(Array<CommandID>& commands) override;;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform(const InvocationInfo &info) override;

	void paint(Graphics &g);

	void previewFile(const File& f);

	void textEditorReturnKeyPressed(TextEditor& editor) override;

	~FileBrowser();

	bool keyPressed(const KeyPress& key) override;;

	void resized() override;

	void mouseDown(const MouseEvent& event) override;

	void mouseDoubleClick(const MouseEvent& event) override;

private:

	class UndoableBrowseAction : public UndoableAction
	{
	public:

		UndoableBrowseAction(FileBrowser *browser_, File newDirectory):
			currentDirectory(newDirectory),
			browser(browser_)
		{
			lastDirectory = browser->directoryList->getDirectory();
		}

		bool perform() override
		{
            browser->goToDirectory(currentDirectory, false);
			
			return true;
		}

		bool undo() override
		{
            browser->goToDirectory(lastDirectory, false);
            
            return true;
		}

	private:

		File lastDirectory;
		File currentDirectory;

		FileBrowser *browser;
	};

	class AudioFileFilter : public FileFilter
	{
	public:

		AudioFileFilter() :
			FileFilter("AudioFileFilter")
		{};

		bool isFileSuitable(const File &file) const override
		{
			return file.hasFileExtension("hip") || AudioSampleProcessorBufferComponent::isAudioFile(file.getFullPathName());
		}
		

		bool isDirectorySuitable(const File &) const override
		{
			return true;
		}

	};
    
    class HiseFileBrowserFilter: public FileFilter
    {
    public:
        HiseFileBrowserFilter():
            FileFilter("HiseFileBrowserFilter")
        {
            
        };
        
        bool isFileSuitable(const File &file) const override
        {
#if JUCE_WINDOWS
			if (file.getFileName().startsWith(".")) return false; // skip OSX hidden files on windows
#endif

			return file.hasFileExtension("hip") ||

				file.hasFileExtension("ttf") ||
#if JUCE_WINDOWS
				file.getFileName() == "LinkWindows" ||
#else
				file.getFileName() == "LinkOSX" ||
#endif
				isAudioFile(file) || isImageFile(file) || isXmlFile(file) || isScriptFile(file);
        }
        
		bool isImageFile(const File& file) const
		{
			return ImageFileFormat::findImageFormatForFileExtension(file) != nullptr;
		}

		bool isAudioFile(const File& file) const
		{
			return AudioSampleProcessorBufferComponent::isAudioFile(file.getFullPathName());
		}
        
		bool isXmlFile(const File& file) const
		{
			return file.hasFileExtension("xml");
		}

        bool isDirectorySuitable(const File &) const override
        {
            return true;
        }

		bool isScriptFile(const File &f) const
		{
			return f.hasFileExtension("js");
		}
    };
    
    
    void saveFavoriteFile()
    {
        ValueTree v("Favorites");
        
        for(int i = 0; i < favorites.size(); i++)
        {
            v.addChild(favorites[i]->exportAsValueTree(), -1, nullptr);
        }
        
        File favoritesFile = NativeFileHandler::getAppDataDirectory().getChildFile("Favorites.xml");
        
        favoritesFile.replaceWithText(v.toXmlString());
    }
    
    void loadFavoriteFile()
    {
        File favoritesFile = NativeFileHandler::getAppDataDirectory().getChildFile("Favorites.xml");
        
        ScopedPointer<XmlElement> xml = XmlDocument::parse(favoritesFile);
        
        if(xml == nullptr) return;
        
        ValueTree v = ValueTree::fromXml(*xml);
        
        for(int i = 0; i < v.getNumChildren(); i++)
        {
            favorites.add(new Favorite("", File()));
        
            favorites.getLast()->restoreFromValueTree(v.getChild(i));
        }
    }

    FileBrowserLookAndFeel laf;
    PopupLookAndFeel plaf;
    
    TimeSliceThread directorySearcher;
    
	File currentlyPreviewFile;
    

	friend class FileBrowserToolbarFactory;

	

	ScopedPointer<DirectoryContentsList> directoryList;

	

	ScopedPointer<FileTreeComponent> fileTreeComponent;

	ScopedPointer<ShapeButton> favoriteButton;
	ScopedPointer<ShapeButton> addToFavoriteButton;
	ScopedPointer<ShapeButton> removeFromFavoritesButton;

	ScopedPointer<FileFilter> fileFilter;

	Component::SafePointer<BackendRootWindow> rootWindow;

	var fileArray;

	

	ScopedPointer<UndoManager> browseUndoManager;
	ScopedPointer<ApplicationCommandManager> browserCommandManager;

	ScopedPointer<Toolbar> browserToolbar;
	ScopedPointer<ToolbarItemFactory> browserToolbarFactory;

	ScopedPointer<TextEditor> textEditor;

	OwnedArray<Favorite> favorites;
};

} // namespace hise

#endif  // FILEBROWSER_H_INCLUDED
