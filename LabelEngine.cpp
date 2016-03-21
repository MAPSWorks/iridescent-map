#include "LabelEngine.h"
#include <iostream>
using namespace std;

LabelDef::LabelDef()
{

}

LabelDef::LabelDef(const class LabelRect &labelRect,
		const class TextProperties &foregroundProp,
		const class TextProperties &backgroundProp,
		const std::vector<class TextLabel> &labels) : labelRect(labelRect),
		foregroundProp(foregroundProp),
		backgroundProp(backgroundProp),
		labels(labels)
{

}

LabelDef::LabelDef(const class LabelDef &a)
{
	*this = a;
}

LabelDef::~LabelDef()
{

}

LabelDef& LabelDef::operator=(const LabelDef &arg)
{
	labelRect = arg.labelRect;
	foregroundProp = arg.foregroundProp;
	backgroundProp = arg.backgroundProp;
	labels = arg.labels;
}

// **********************************************

void MergeLabelsByImportance(LabelsByImportance &mergeIntoThis, const LabelsByImportance &labelsToMerge)
{
	for(LabelsByImportance::const_iterator itr = labelsToMerge.begin(); itr != labelsToMerge.end(); itr++)
	{
		int importance = itr->first;
		const vector<class LabelDef> &lbs = itr->second;
		for(size_t i=0; i<lbs.size(); i++)
		{
			LabelsByImportance::iterator mergeIt = mergeIntoThis.find(importance);
			if(mergeIt == mergeIntoThis.end())
				mergeIntoThis[importance] = vector<class LabelDef>();
			mergeIntoThis[importance].push_back(lbs[i]);
		}
	}
}

// **********************************************

LabelRect::LabelRect() : x(0.0), y(0.0), w(1.0), h(1.0)
{
	
}

LabelRect::LabelRect(double x, double y, double w, double h) : x(x), y(y), w(w), h(h)
{

}

LabelRect::LabelRect(const class LabelRect &a)
{
	*this = a;
}

LabelRect::~LabelRect()
{

}

LabelRect& LabelRect::operator=(const LabelRect &arg)
{
	x = arg.x;
	y = arg.y;
	w = arg.w;
	h = arg.h;
	return *this;
}

bool LabelRect::Overlaps(const LabelRect &arg) const
{
	if (x + w < arg.x) return false;
	if (x >= arg.x + arg.w) return false;
	if (y + h < arg.y) return false;
	if (y >= arg.y + arg.w) return false;
	return true;
}

void LabelRect::Print() const
{
	cout << x << "," << y << "," << w << "," << h << endl;
}


// ***************************************************

PoiLabel::PoiLabel()
{
	sx = 0.0;
	sy = 0.0;
	importance = 0;
}

PoiLabel::PoiLabel(double sx, double sy, const std::string &textName, const TagMap &tags, int importance):
	sx(sx), sy(sy), textName(textName), tags(tags), importance(importance)
{
	
}

PoiLabel::PoiLabel(const class PoiLabel &a)
{
	*this = a;
}

PoiLabel::~PoiLabel()
{

}

PoiLabel& PoiLabel::operator=(const PoiLabel &arg)
{
	sx = arg.sx;
	sy = arg.sy;
	textName = arg.textName;
	tags = arg.tags;
	importance = arg.importance;
}

// *******************************************************

LabelEngine::LabelEngine(class IDrawLib *output):
	output(output)
{

}

LabelEngine::~LabelEngine()
{

}

void LabelEngine::LabelPoisToStyledLabel(std::vector<class PoiLabel> &poiLabels, LabelsByImportance &organisedLabelsOut)
{
	organisedLabelsOut.clear();
	for(size_t i=0;i < poiLabels.size(); i++)
	{
		class PoiLabel &label = poiLabels[i];

		std::string &textName = label.textName;
		std::string outString;
		if(textName[0] == '[' && textName[textName.size()-1] == ']') {
			std::string keyName(&textName[1], textName.size()-2);
			TagMap::const_iterator it = label.tags.find(keyName);
			if(it == label.tags.end()) continue;
			outString = it->second;
		}
		else
			outString = textName;

		//Get bounds
		double width=0.0, height=0.0;
		class TextProperties foregroundProp(1.0,1.0,1.0);
		if(this->output != NULL)
		{			
			int ret = this->output->GetTextExtents(outString.c_str(), foregroundProp, 
				width, height);
			if(ret != 0)
			{
				width=0.0, height=0.0;
			}
		}
	
		std::vector<class TextLabel> textStrs;
		double lx = label.sx - width / 2.0;
		double ly = label.sy;
		textStrs.push_back(TextLabel(outString, lx, ly));

		class LabelRect labelRect(lx, ly, width, height);

		class TextProperties backgroundProp(0.0,0.0,0.0);
		backgroundProp.a = 0.5;
		backgroundProp.outline = true;
		backgroundProp.lineWidth=2.0;

		//Add label definition to list
		LabelsByImportance::iterator it = organisedLabelsOut.find(label.importance);
		if(it == organisedLabelsOut.end())
			organisedLabelsOut[label.importance] = vector<class LabelDef>();
		organisedLabelsOut[label.importance].push_back(LabelDef(labelRect, foregroundProp, backgroundProp, textStrs));
	}
}

bool LabelOverlaps(const class LabelDef &label, const LabelsByImportance existingLabels)
{
	for(LabelsByImportance::const_iterator itr = existingLabels.begin(); itr != existingLabels.end(); itr++)
	{
		const vector<class LabelDef> &lbs = itr->second;
		for(size_t j=0; j<lbs.size(); j++)
		{
			if(label.labelRect.Overlaps(lbs[j].labelRect))
				return true;
		}
	}
	return false;
}

void LabelEngine::RemoveOverlapping(const LabelsByImportance &organisedLabelsTmp, LabelsByImportance &organisedLabelsOut)
{
	organisedLabelsOut.clear();
	
	//Starting with high imporance labels, check for overlaps
	if(organisedLabelsTmp.size() == 0)
		return;
	LabelsByImportance::const_iterator itr = organisedLabelsTmp.end();
	itr --;
	bool looping = true;
	while(looping)
	{
		if(itr == organisedLabelsTmp.begin())
			looping = false;
		int importance = itr->first;
		const vector<class LabelDef> &lbs = itr->second;
		for(size_t j=0; j<lbs.size(); j++)
		{
			const class LabelDef &label = lbs[j];
			bool overlaps = LabelOverlaps(label, organisedLabelsOut);
			if(!overlaps)
			{
				LabelsByImportance::iterator chkIt = organisedLabelsOut.find(importance);
				if(chkIt == organisedLabelsOut.end())
					organisedLabelsOut[importance] = vector<class LabelDef>();
				organisedLabelsOut[importance].push_back(label);
			}
		}
		if(looping)
			itr --;
	}
}

void LabelEngine::WriteDrawCommands(const LabelsByImportance &organisedLabels)
{

	for(LabelsByImportance::const_iterator itr = organisedLabels.begin(); itr != organisedLabels.end(); itr++)
	{
		const vector<class LabelDef> &lbs = itr->second;
		for(size_t j=0; j<lbs.size(); j++)
		{
			const class LabelDef &labelDef = lbs[j];

			//Ghost background
			if(this->output != NULL)
				this->output->AddDrawTextCmd(labelDef.labels, labelDef.backgroundProp);

			//Foreground text
			if(this->output != NULL)
				this->output->AddDrawTextCmd(labelDef.labels, labelDef.foregroundProp);
		}
	}
}


