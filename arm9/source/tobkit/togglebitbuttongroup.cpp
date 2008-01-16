#include "togglebitbutton.h"

ToggleBitButton::ToggleBitButtonGroup::ToggleBitButtonGroup(bool _forceSelection):
	forceSelection(_forceSelection)
{
	onChange = 0;
}

void ToggleBitButton::ToggleBitButtonGroup::add(ToggleBitButton *rb) {
	rbvec.push_back(rb);
}

void ToggleBitButton::ToggleBitButtonGroup::pushed(ToggleBitButton *rb)
{
	u8 counter = 0, pos = 0;
	std::vector<ToggleBitButton*>::iterator rbit;
	
	for(rbit=rbvec.begin(); rbit!=rbvec.end(); ++rbit) {
		if(*rbit != rb) {
			(*rbit)->setState(false);
		} else {
			if( ((*rbit)->getState() == true) && (!forceSelection) )
			{
				(*rbit)->setState(false);
				pos = -1;
			}
			else
			{
				(*rbit)->setState(true);
				pos = counter;
			}
		}
		counter++;
	}
	
	if(onChange) {
		onChange(pos);
	}
}

void ToggleBitButton::ToggleBitButtonGroup::setActive(s8 idx)
{
	if(idx >= 0)
	{
		pushed(rbvec.at(idx));
	}
	else if(forceSelection == false)
	{
		std::vector<ToggleBitButton*>::iterator rbit;
		for(rbit=rbvec.begin(); rbit!=rbvec.end(); ++rbit)
			(*rbit)->setState(false);
			
		if(onChange)
			onChange(-1);
	}
}

void ToggleBitButton::ToggleBitButtonGroup::registerChangeCallback(void (*onChange_)(s8)) {
	onChange = onChange_;
}
