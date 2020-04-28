/*
	 ------------------------------------------------------------------

	 This file is part of the Open Ephys GUI
	 Copyright (C) 2019 Open Ephys

	 ------------------------------------------------------------------

	 This program is free software: you can redistribute it and/or modify
	 it under the terms of the GNU General Public License as published by
	 the Free Software Foundation, either version 3 of the License, or
	 (at your option) any later version.

	 This program is distributed in the hope that it will be useful,
	 but WITHOUT ANY WARRANTY; without even the implied warranty of
	 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	 GNU General Public License for more details.

	 You should have received a copy of the GNU General Public License
	 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include "PluginInstaller.h"
#include <stdio.h>

#include "../CoreServices.h"
//-----------------------------------------------------------------------

static inline File getPluginsLocationDirectory() {
#if defined(__APPLE__)
    File dir = File::getSpecialLocation(File::userApplicationDataDirectory).getChildFile("Application Support/open-ephys");
    if (!dir.isDirectory()) {
        dir.createDirectory();
    }
    return std::move(dir);
#else
    return File::getSpecialLocation(File::currentExecutableFile).getParentDirectory();
#endif
}

static String osType;

PluginInstaller::PluginInstaller(MainWindow* mainWindow)
: DocumentWindow(WINDOW_TITLE,
		Colour(Colours::black),
		DocumentWindow::closeButton)
{

	MouseCursor::showWaitCursor();
	parent = (DocumentWindow*)mainWindow;

	setResizable(
        true,  // isResizable
		false); // useBottomCornerRisizer -- doesn't work very well

    //TODO: Add command manager for hot-key functionality later...

    /*
	commandManager.registerAllCommandsForTarget(ui);
	commandManager.registerAllCommandsForTarget(JUCEApplication::getInstance());
	ui->setApplicationCommandManagerToWatch(&commandManager);
	addKeyListener(commandManager.getKeyMappings());
    */

   // Identify the OS on which the GUI is running
	SystemStats::OperatingSystemType os = SystemStats::getOperatingSystemType();

	if ((os & SystemStats::OperatingSystemType::Windows) != 0)
	 	osType = "windows";
	else if ((os & SystemStats::OperatingSystemType::MacOSX) != 0)
	 	osType = "mac";
	else if ((os & SystemStats::OperatingSystemType::Linux) != 0)
	 	osType = "linux";

	//Initialize Plugin Installer Components
	setUsingNativeTitleBar(true);
	setContentOwned(new PluginInstallerComponent(), false);
	setVisible(true);
	
	int x = parent->getX();
	int y = parent->getY();
	int w = parent->getWidth();
	int h = parent->getHeight();

	setBounds(x + (0.5*w) - 427, y + 0.5*h - 240, 854, 480);

	// Constraining the window's size doesn't seem to work:
	setResizeLimits(640, 480, 8192, 5120);

	createXmlFile();

	MouseCursor::hideWaitCursor();
	CoreServices::sendStatusMessage("Plugin Installer is ready!");
}

PluginInstaller::~PluginInstaller()
{
	masterReference.clear();
}

void PluginInstaller::closeButtonPressed()
{
	setVisible(false);
	delete this;
}

void PluginInstaller::createXmlFile()
{
	String xmlFile = "plugins" + File::separatorString + "installedPlugins.xml";
	File file = getPluginsLocationDirectory().getChildFile(xmlFile);

	XmlDocument doc(file);
	std::unique_ptr<XmlElement> xml (doc.getDocumentElement());

	if (xml == 0 || ! xml->hasTagName("PluginInstaller"))
	{
		std::unique_ptr<XmlElement> baseTag(new XmlElement("PluginInstaller"));
		baseTag->setAttribute("gui_version", JUCEApplication::getInstance()->getApplicationVersion());

		std::unique_ptr<XmlElement> plugins(new XmlElement("InstalledPlugins"));

		baseTag->addChildElement(plugins.release());

		if (! baseTag->writeToFile(file, String::empty))
			std::cout << "Error! Couldn't write to installedPlugins.xml" << std::endl;
	}
}

/* ================================== Plugin Installer Component ================================== */

PluginInstallerComponent::PluginInstallerComponent()
{
	font = Font("FiraSans", 18, Font::plain);
	setSize(getWidth() - 10, getHeight() - 10);

	addAndMakeVisible(pluginListAndInfo);

	addAndMakeVisible(sortingLabel);
	sortingLabel.setColour(Label::textColourId, Colours::white);
	sortingLabel.setFont(font);
	sortingLabel.setText("Sort By:", dontSendNotification);

	addAndMakeVisible(sortByMenu);
	sortByMenu.setJustificationType(Justification::centred);
	sortByMenu.addItem("A - Z", 1);
	sortByMenu.addItem("Z - A", 2);
	sortByMenu.setTextWhenNothingSelected("-----");
	sortByMenu.addListener(this);

	addAndMakeVisible(filterLabel);
	filterLabel.setColour(Label::textColourId, Colours::white);
	filterLabel.setFont(font);
	filterLabel.setText("View:", dontSendNotification);

	addAndMakeVisible(allLabel);
	allLabel.setButtonText("All");
	allLabel.setColour(ToggleButton::textColourId, Colours::white);
	allLabel.setRadioGroupId(101, dontSendNotification);
	allLabel.addListener(this);	
	allLabel.setToggleState(true, dontSendNotification);

	addAndMakeVisible(installedLabel);
	installedLabel.setButtonText("Installed");
	installedLabel.setColour(ToggleButton::textColourId, Colours::white);
	installedLabel.setRadioGroupId(101, dontSendNotification);
	installedLabel.addListener(this);

	addAndMakeVisible(updatesLabel);
	updatesLabel.setButtonText("Updates");
	updatesLabel.setColour(ToggleButton::textColourId, Colours::white);
	updatesLabel.setRadioGroupId(101, dontSendNotification);
	updatesLabel.addListener(this);
}

void PluginInstallerComponent::paint(Graphics& g)
{
	g.fillAll (Colours::darkgrey);
	g.setColour(Colours::lightgrey);
	g.fillRect(192, 5, 3, 38);
}

void PluginInstallerComponent::resized()
{
	sortingLabel.setBounds(20, 10, 70, 30);
	sortByMenu.setBounds(90, 10, 90, 30);

	filterLabel.setBounds(200, 10, 50, 30);
	allLabel.setBounds(250, 10, 50, 30);
	installedLabel.setBounds(300, 10, 90, 30);
	updatesLabel.setBounds(390, 10, 90, 30);

	pluginListAndInfo.setBounds(10, 40, getWidth() - 10, getHeight() - 40);
}

void PluginInstallerComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged->getSelectedId() == 1)
	{
		pluginListAndInfo.pluginArray.sort(true);
		pluginListAndInfo.repaint();
	}
	else if (comboBoxThatHasChanged->getSelectedId() == 2)
	{
		pluginListAndInfo.pluginArray.sort(true);
		int size = pluginListAndInfo.pluginArray.size();
		for (int i = 0; i < size / 2; i++)
		{
			pluginListAndInfo.pluginArray.getReference(i).swapWith
			(pluginListAndInfo.pluginArray.getReference(size - i - 1));
		}

		pluginListAndInfo.repaint();
	}
}

void PluginInstallerComponent::loadInstalledPluginNames()
{
	String fileStr = "plugins" + File::separatorString + "installedPlugins.xml";
	File xmlFile = getPluginsLocationDirectory().getChildFile(fileStr);

	XmlDocument doc(xmlFile);
	std::unique_ptr<XmlElement> xml (doc.getDocumentElement());

	if (xml == 0 || ! xml->hasTagName("PluginInstaller"))
	{
		std::cout << "[PluginInstaller] File not found." << std::endl;
		return;
	}
	else
	{	
		installedPlugins.clear();
		updatablePlugins.clear();
		auto child = xml->getFirstChildElement();

		String baseUrl = "https://api.bintray.com/packages/open-ephys-gui-plugins/";

		forEachXmlChildElement(*child, e)
		{
			String pName = e->getTagName();
			installedPlugins.add(pName);

			//Get latest version
			String versionUrl = baseUrl + pName + "/" + pName + "-" + osType + "/versions/_latest";

			String vResponse = URL(versionUrl).readEntireTextStream();
			var vReply = JSON::parse(vResponse);

			String latest_ver = vReply.getProperty("name", "NULL").toString();

			if (!latest_ver.equalsIgnoreCase(e->getAttributeValue(0)))
				updatablePlugins.add(pName);
		}
	}
}

void PluginInstallerComponent::buttonClicked(Button* button)
{
	if(allPlugins.isEmpty())
	{
		allPlugins.addArray(pluginListAndInfo.pluginArray);
	}

	if(button == &installedLabel)
	{
		if(installedPlugins.isEmpty())
			loadInstalledPluginNames();
		
		pluginListAndInfo.pluginArray.clear();
		pluginListAndInfo.pluginArray.addArray(installedPlugins);
		pluginListAndInfo.setNumRows(installedPlugins.size());
	}
	else if(button == &allLabel)
	{	
		pluginListAndInfo.pluginArray.clear();
		pluginListAndInfo.pluginArray.addArray(allPlugins);
		pluginListAndInfo.setNumRows(allPlugins.size());
	}
	else if(button == &updatesLabel)
	{
		if(installedPlugins.isEmpty())
			loadInstalledPluginNames();
		
		pluginListAndInfo.pluginArray.clear();
		pluginListAndInfo.pluginArray.addArray(updatablePlugins);
		pluginListAndInfo.setNumRows(updatablePlugins.size());
	}

	sortByMenu.setSelectedId(-1, dontSendNotification);
}


/* ================================== Plugin Table Component ================================== */

PluginListBoxComponent::PluginListBoxComponent()
{
	listFont = Font("FiraSans Bold", 22, Font::plain);

	loadAllPluginNames();

	addAndMakeVisible(pluginList);
	pluginList.setModel(this);
	pluginList.setColour(ListBox::backgroundColourId , Colours::grey);
	pluginList.setRowHeight(35);
	pluginList.setMouseMoveSelectsRows(true);

	addAndMakeVisible(pluginInfoPanel);
}

int PluginListBoxComponent::getNumRows()
{
	return numRows;
}

void PluginListBoxComponent::paintListBoxItem (int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	if (rowIsSelected)
	{
		g.fillAll(Colour::fromRGBA(238, 238, 238, 100));
		g.setColour (Colours::darkgrey);
	}
	else
	{
		g.fillAll(Colours::grey);
		g.setColour (Colours::white);
	}

	if ( rowNumber == pluginArray.indexOf(lastPluginSelected, true, 0) )
	{
		g.setColour (Colours::yellow);
	}

	g.setFont(listFont);

	String text = pluginArray[rowNumber];

	g.drawText (text, 20, 0, width - 10, height, Justification::centredLeft, true);
}

void PluginListBoxComponent::loadAllPluginNames()
{
	/* Get list of plugins uploaded to bintray */
	String response = URL("https://api.bintray.com/repos/open-ephys-gui-plugins").readEntireTextStream();

	var pluginData = JSON::parse(response);

	numRows = pluginData.size();
	
	// std::cout << "jsonReply" << response.bodyAsString << std::endl;

	String pluginName;

	int pluginTextWidth;

	for (int i = 0; i < numRows; i++)
	{
		//Array<String> packages;
		
		pluginName = pluginData[i].getProperty("name", var()).toString();
		pluginArray.add(pluginName);

		pluginTextWidth = listFont.getStringWidth(pluginName);
		if (pluginTextWidth > maxTextWidth)
			maxTextWidth = pluginTextWidth;
	}
}

bool PluginListBoxComponent::loadPluginInfo(const String& pluginName)
{
	// Find out all available packages for the plugin
	String url, version_url;
	
	url = "https://api.bintray.com/repos/open-ephys-gui-plugins/";
	url+=pluginName;
	url+="/packages";

	String packageResponse = URL(url).readEntireTextStream();

	var packageReply = JSON::parse(packageResponse);

	Array<String> packages;

	for (int i = 0; i < packageReply.size(); i++)
	{
	 	packages.add(packageReply[i].getProperty("name", var()).toString());
	}
		
	// Select platform specific package for the plugin
	String selectedPackage;
	for (int i = 0; i < packages.size(); i++)
	{
	 	if(packages[i].contains(osType))
	 	{
	 		selectedPackage = packages[i];
	 		break;
	 	}
	}

	if(selectedPackage.isEmpty())
	{
	 	std::cout << "*********** No platform specific package found for " << pluginName << std::endl;
		pluginInfoPanel.makeInfoVisible(false);
		return false;
	}

	//Get latest version
	version_url = "https://api.bintray.com/packages/open-ephys-gui-plugins/";
	version_url+=pluginName;
	version_url+="/";
	version_url+=selectedPackage;

	String version_response = URL(version_url).readEntireTextStream();;

	var version_reply = JSON::parse(version_response);

	String owner= version_reply.getProperty("owner", "NULL");
	String latest_version = version_reply.getProperty("latest_version", "NULL");
	String updated = version_reply.getProperty("updated", "NULL");
	String description = version_reply.getProperty("desc", "NULL");

	auto allVersions = version_reply.getProperty("versions", "NULL").getArray();

	selectedPluginInfo.versions.clear();

	for (String version : *allVersions)
		selectedPluginInfo.versions.add(version);

	selectedPluginInfo.docURL = version_reply.getProperty("vcs_url", "NULL").toString();
	selectedPluginInfo.selectedVersion = String();

	selectedPluginInfo.pluginName = pluginName;
	selectedPluginInfo.packageName = selectedPackage;
	selectedPluginInfo.owner = owner;
	selectedPluginInfo.latestVersion = latest_version;
	selectedPluginInfo.lastUpdated = updated;
	selectedPluginInfo.description = description;
	selectedPluginInfo.dependencies = " ";

	// If the plugin is already installed, get installed version number
	String fileStr = "plugins" + File::separatorString + "installedPlugins.xml";
	File xmlFile = getPluginsLocationDirectory().getChildFile(fileStr);

	XmlDocument doc(xmlFile);
	std::unique_ptr<XmlElement> xml (doc.getDocumentElement());

	if (xml == 0 || ! xml->hasTagName("PluginInstaller"))
	{
		std::cout << "[PluginInstaller] File not found." << std::endl;
		return false;
	}
	else
	{	
		auto child = xml->getFirstChildElement();

		auto pluginEntry = child->getChildByName(pluginName);

		if (pluginEntry != nullptr)
			selectedPluginInfo.installedVersion = pluginEntry->getAttributeValue(0);
		else
			selectedPluginInfo.installedVersion = String::empty;
		
	}

	pluginInfoPanel.setPluginInfo(selectedPluginInfo);
	pluginInfoPanel.makeInfoVisible(true);

	return true;
}

void PluginListBoxComponent::listBoxItemClicked (int row, const MouseEvent &)
{
	this->returnKeyPressed(row);
}

void PluginListBoxComponent::resized()
{
	// position our table with a gap around its edge
    pluginList.setBounds(10, 10, maxTextWidth + 60, getHeight() - 30);
	pluginInfoPanel.setBounds(maxTextWidth + 80, 10, 
							  getWidth() - maxTextWidth - 100, getHeight() - 30);
}

void PluginListBoxComponent::returnKeyPressed (int lastRowSelected)
{
	if (!lastPluginSelected.equalsIgnoreCase(pluginArray[lastRowSelected]))
	{
		lastPluginSelected = pluginArray[lastRowSelected];

		pluginInfoPanel.makeInfoVisible(false);
		pluginInfoPanel.updateStatusMessage("Loading Plugin Info...", true);
		
		if(loadPluginInfo(lastPluginSelected))
			pluginInfoPanel.updateStatusMessage("", false);
		else
			pluginInfoPanel.updateStatusMessage("No platform specific package found for " + lastPluginSelected, true);
		
		this->repaint();
	}
}

/* ================================== Plugin Information Component ================================== */

PluginInfoComponent::PluginInfoComponent()
{
	infoFont = Font("FiraSans", 20, Font::plain);
	infoFontBold = Font("FiraSans Bold", 20, Font::plain);
	
	addChildComponent(pluginNameLabel);
	pluginNameLabel.setFont(infoFontBold);
	pluginNameLabel.setColour(Label::textColourId, Colours::white);
	pluginNameLabel.setText("Name: ", dontSendNotification);

	addChildComponent(pluginNameText);
	pluginNameText.setFont(infoFont);
	pluginNameText.setColour(Label::textColourId, Colours::white);

	addChildComponent(ownerLabel);
	ownerLabel.setFont(infoFontBold);
	ownerLabel.setColour(Label::textColourId, Colours::white);
	ownerLabel.setText("Owner: ", dontSendNotification);

	addChildComponent(ownerText);
	ownerText.setFont(infoFont);
	ownerText.setColour(Label::textColourId, Colours::white);

	addChildComponent(versionLabel);
	versionLabel.setFont(infoFontBold);
	versionLabel.setColour(Label::textColourId, Colours::white);
	versionLabel.setText("Version: ", dontSendNotification);

	addChildComponent(versionMenu);
	versionMenu.setJustificationType(Justification::centred);
	versionMenu.setTextWhenNoChoicesAvailable("- N/A -");
	versionMenu.addListener(this);

	addChildComponent(lastUpdatedLabel);
	lastUpdatedLabel.setFont(infoFontBold);
	lastUpdatedLabel.setColour(Label::textColourId, Colours::white);
	lastUpdatedLabel.setText("Last Updated: ", dontSendNotification);

	addChildComponent(lastUpdatedText);
	lastUpdatedText.setFont(infoFont);
	lastUpdatedText.setColour(Label::textColourId, Colours::white);

	addChildComponent(descriptionLabel);
	descriptionLabel.setFont(infoFontBold);
	descriptionLabel.setColour(Label::textColourId, Colours::white);
	descriptionLabel.setText("Description: ", dontSendNotification);

	addChildComponent(descriptionText);
	descriptionText.setFont(infoFont);
	descriptionText.setColour(Label::textColourId, Colours::white);
	descriptionText.setJustificationType(Justification::top);
	descriptionText.setMinimumHorizontalScale(1.0f);

	addChildComponent(dependencyLabel);
	dependencyLabel.setFont(infoFontBold);
	dependencyLabel.setColour(Label::textColourId, Colours::white);
	dependencyLabel.setText("Dependencies: ", dontSendNotification);

	addChildComponent(dependencyText);
	dependencyText.setFont(infoFont);
	dependencyText.setColour(Label::textColourId, Colours::white);

	addChildComponent(downloadButton);
	downloadButton.setButtonText("Download");
	downloadButton.setColour(TextButton::buttonColourId, Colours::lightgrey);
	downloadButton.addListener(this);

	addChildComponent(documentationButton);
	documentationButton.setButtonText("Open Documentation");
	documentationButton.setColour(TextButton::buttonColourId, Colours::lightgrey);
	documentationButton.addListener(this);

	addAndMakeVisible(statusLabel);
	statusLabel.setFont(infoFont);
	statusLabel.setColour(Label::textColourId, Colours::white);
	statusLabel.setText("Please select a plugin from the list on the left...", dontSendNotification);
}

void PluginInfoComponent::paint(Graphics& g)
{
	g.fillAll (Colours::grey);
}

void PluginInfoComponent::resized()
{
	pluginNameLabel.setBounds(10, 30, 60, 30);
	pluginNameText.setBounds(125, 30, getWidth() - 10, 30);

	ownerLabel.setBounds(10, 60, 60, 30);
	ownerText.setBounds(125, 60, getWidth() - 10, 30);

	versionLabel.setBounds(10, 90, 80, 30);
	versionMenu.setBounds(130, 90, 110, 30);

	lastUpdatedLabel.setBounds(10, 120, 120, 30);
	lastUpdatedText.setBounds(125, 120, getWidth() - 10, 30);

	descriptionLabel.setBounds(10, 150, 110, 30);
	descriptionText.setBounds(125, 155, getWidth() - 130, 75);

	dependencyLabel.setBounds(10, 160 + descriptionText.getHeight(), 120, 30);
	dependencyText.setBounds(125, dependencyLabel.getY(), getWidth() - 10, 30);

	downloadButton.setBounds(getWidth() - (getWidth() * 0.4) - 20, getHeight() - 60, getWidth() * 0.4, 30);
	documentationButton.setBounds(20, getHeight() - 60, getWidth() * 0.4, 30);
	
	statusLabel.setBounds(10, (getHeight() / 2) - 15, getWidth() - 10, 30);
}

void PluginInfoComponent::buttonClicked(Button* button)
{
	if (button == &downloadButton)
	{
		std::cout << "Downloading Plugin: " << pInfo.pluginName << "...  ";
		
		bool dlSucess = downloadPlugin(pInfo.pluginName, pInfo.packageName, pInfo.selectedVersion);

		if(dlSucess)
		{	
			AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, 
												   "Plugin Installer " + pInfo.pluginName, 
												   pInfo.pluginName + " Installed Successfully");

			std::cout << "Download Successfull!!" << std::endl;

			downloadButton.setEnabled(false);
			downloadButton.setButtonText("Installed");
		}
		else
		{
			AlertWindow::showMessageBoxAsync(AlertWindow::WarningIcon, 
												   "Plugin Installer " + pInfo.pluginName, 
												   "Error Installing " + pInfo.pluginName);

			std::cout << "Download Failed!!" << std::endl;
		}
		
	}
	else if (button == &documentationButton)
	{
		URL url = URL(pInfo.docURL);
		url.launchInDefaultBrowser();
	}
}

int PluginInfoComponent::versionCompare(const String& v1, const String& v2)
{ 
    //  vnum stores each numeric part of version 
    int vnum1 = 0, vnum2 = 0; 
  
    //  loop untill both string are processed 
    for (int i=0, j=0; (i<v1.length() || j<v2.length()); ) 
    { 
        //  storing numeric part of version 1 in vnum1 
        while (i < v1.length() && v1[i] != '.') 
        { 
            vnum1 = vnum1 * 10 + (v1[i] - '0'); 
            i++; 
        } 
  
        //  storing numeric part of version 2 in vnum2 
        while (j < v2.length() && v2[j] != '.') 
        { 
            vnum2 = vnum2 * 10 + (v2[j] - '0'); 
            j++; 
        } 
  
        if (vnum1 > vnum2) 
            return 1; 
        if (vnum2 > vnum1) 
            return -1; 
  
        //  if equal, reset variables and go for next numeric 
        // part 
        vnum1 = vnum2 = 0; 
        i++; 
        j++; 
    } 
    return 0; 
} 

void PluginInfoComponent::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
	if (comboBoxThatHasChanged == &versionMenu)
	{
		pInfo.selectedVersion = comboBoxThatHasChanged->getText();

		if (pInfo.installedVersion.isEmpty())
		{
			downloadButton.setEnabled(true);
			downloadButton.setButtonText("Download");
		}
		else
		{
			int result = versionCompare(pInfo.selectedVersion, pInfo.installedVersion);

			if (result == 0)
			{
				downloadButton.setEnabled(false);
				downloadButton.setButtonText("Installed");
			}
			else if (result > 0)
			{
				downloadButton.setEnabled(true);
				downloadButton.setButtonText("Upgrade");
			}
			else
			{
				downloadButton.setEnabled(true);
				downloadButton.setButtonText("Downgrade");
			}
		}	
	}
}

void PluginInfoComponent::setPluginInfo(const SelectedPluginInfo& p)
{
	pInfo = p;
	pluginNameText.setText(pInfo.pluginName, dontSendNotification);
	ownerText.setText(pInfo.owner, dontSendNotification);
	lastUpdatedText.setText(pInfo.lastUpdated, dontSendNotification);
	descriptionText.setText(pInfo.description, dontSendNotification);
	dependencyText.setText(pInfo.dependencies, dontSendNotification);

	versionMenu.clear(dontSendNotification);

	for (int i = 0; i < pInfo.versions.size(); i++)
		versionMenu.addItem(pInfo.versions[i], i + 1);

	//set default selected version to the first entry in combo box
	versionMenu.setSelectedId(1, sendNotification);
	pInfo.selectedVersion = pInfo.versions[0];
}

void PluginInfoComponent::updateStatusMessage(const String& str, bool isVisible)
{
	statusLabel.setText(str, dontSendNotification);
	statusLabel.setVisible(isVisible);
}

void PluginInfoComponent::makeInfoVisible(bool isEnabled)
{
	pluginNameLabel.setVisible(isEnabled);
	pluginNameText.setVisible(isEnabled);

	ownerLabel.setVisible(isEnabled);
	ownerText.setVisible(isEnabled);

	versionLabel.setVisible(isEnabled);
	versionMenu.setVisible(isEnabled);

	lastUpdatedLabel.setVisible(isEnabled);
	lastUpdatedText.setVisible(isEnabled);

	descriptionLabel.setVisible(isEnabled);
	descriptionText.setVisible(isEnabled);

	dependencyLabel.setVisible(isEnabled);
	dependencyText.setVisible(isEnabled);

	downloadButton.setVisible(isEnabled);
	documentationButton.setVisible(isEnabled);
}

bool PluginInfoComponent::downloadPlugin(const String& plugin, const String& package, const String& version) 
{

	String fileStr = "plugins" + File::separatorString + "installedPlugins.xml";
	File xmlFile = getPluginsLocationDirectory().getChildFile(fileStr);

	XmlDocument doc(xmlFile);
	std::unique_ptr<XmlElement> xml (doc.getDocumentElement());

	std::unique_ptr<XmlElement> pluginEntry(new XmlElement(plugin));
	pluginEntry->setAttribute("version", version);

	if (xml == 0 || ! xml->hasTagName("PluginInstaller"))
	{
		std::cout << "[PluginInstaller] File not found." << std::endl;
		return false;
	}
	else
	{	
		auto child = xml->getFirstChildElement();

		forEachXmlChildElement(*child, e)
		{
			if (e->hasTagName(pluginEntry->getTagName()) &&
				e->getAttributeValue(0).equalsIgnoreCase(pluginEntry->getAttributeValue(0)))
			{
				std::cout << plugin << " v" << version << " already exists!!" << std::endl;
				return false;
			}
		}

		child->addChildElement(pluginEntry.release());

		if (! xml->writeToFile(xmlFile, String::empty))
		{
			std::cout << "Error! Couldn't write to installedPlugins.xml" << std::endl;
			return false;
		}
	}

	
	String files_url = "https://api.bintray.com/packages/open-ephys-gui-plugins/";
	files_url += plugin;
	files_url += "/";
	files_url += package;
	files_url += "/versions/";
	files_url += version;
	files_url += "/files";

	String files_response = URL(files_url).readEntireTextStream();;

	var files_reply = JSON::parse(files_response);

	String filename;

	if (files_reply.size())
	{
		for (int i = 0; i < files_reply.size(); i++)
		{
			filename = files_reply[i].getProperty("name", "NULL").toString();
		}

		//Unzip plugin and install in plugins directory
		//curl -L https://dl.bintray.com/$bintrayUser/$repo/$filename
		String fileDownloadURL = "https://dl.bintray.com/open-ephys-gui-plugins/";
		fileDownloadURL += plugin;
		fileDownloadURL += "/";
		fileDownloadURL += filename;

		URL fileUrl(fileDownloadURL);

		//Create input stream from the plugin's zip file URL
		ScopedPointer<InputStream> fileStream = fileUrl.createInputStream(false);

		//Get path to plugins directory
		File pluginsPath = getPluginsLocationDirectory();

		//Construct path for downloaded zip file
		String pluginFilePath = pluginsPath.getFullPathName();
		pluginFilePath += File::separatorString;
		pluginFilePath += filename;

		//Create local file
		File pluginFile(pluginFilePath);
		pluginFile.deleteFile();

		//Use the Url's input stream and write it to a file using output stream
		FileOutputStream* out = pluginFile.createOutputStream();
		out->writeFromInputStream(*fileStream, -1);
		out->flush();
		delete out;

		ZipFile pluginZip(pluginFile);
		Result rs = pluginZip.uncompressTo(pluginsPath);

		pluginFile.deleteFile();		

		if (rs.failed())
		{
			std::cout << "Uncompressing plugin zip file failed!!" << std::endl;
			return false;
		}
		
		return true;

	}
	//TODO: Prompt user to restart to see plugin in ProcessorList

	return false;
}