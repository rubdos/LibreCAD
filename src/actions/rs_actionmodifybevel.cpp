/****************************************************************************
**
** This file is part of the CADuntu project, a 2D CAD program
**
** Copyright (C) 2010 R. van Twisk (caduntu@rvt.dds.nl)
** Copyright (C) 2001-2003 RibbonSoft. All rights reserved.
**
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software 
** Foundation and appearing in the file gpl-2.0.txt included in the
** packaging of this file.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
** This copyright notice MUST APPEAR in all copies of the script!  
**
**********************************************************************/

#include "rs_actionmodifybevel.h"

#include "rs_snapper.h"
#include "rs_information.h"


RS_ActionModifyBevel::RS_ActionModifyBevel(RS_EntityContainer& container,
        RS_GraphicView& graphicView)
        :RS_PreviewActionInterface("Bevel Entities",
                           container, graphicView) {

    entity1 = NULL;
    coord1 = RS_Vector(false);
    entity2 = NULL;
    coord2 = RS_Vector(false);
}


QAction* RS_ActionModifyBevel::createGUIAction(RS2::ActionType /*type*/, QObject* /*parent*/) {
	// tr("Bevel")
    QAction* action = new QAction(tr("&Bevel"),  NULL);
	action->setIcon(QIcon(":/extui/modifybevel.png"));
    action->setStatusTip(tr("Bevel Entities"));
    return action;
}


void RS_ActionModifyBevel::init(int status) {
    RS_ActionInterface::init(status);

    snapMode = RS2::SnapFree;
    snapRes = RS2::RestrictNothing;
}



void RS_ActionModifyBevel::trigger() {

    RS_DEBUG->print("RS_ActionModifyBevel::trigger()");

    if (entity1!=NULL && entity1->isAtomic() &&
            entity2!=NULL && entity2->isAtomic()) {

        RS_Modification m(*container, graphicView);
        m.bevel(coord1, (RS_AtomicEntity*)entity1,
                coord2, (RS_AtomicEntity*)entity2,
                data);

        entity1 = NULL;
        entity2 = NULL;
        setStatus(SetEntity1);

        RS_DIALOGFACTORY->updateSelectionWidget(container->countSelected());
    }
}



void RS_ActionModifyBevel::mouseMoveEvent(RS_MouseEvent* e) {
    RS_DEBUG->print("RS_ActionModifyBevel::mouseMoveEvent begin");

    RS_Vector mouse = graphicView->toGraph(e->x(), e->y());
    RS_Entity* se = catchEntity(e, RS2::ResolveAll);

    switch (getStatus()) {
    case SetEntity1:
        coord1 = mouse;
        entity1 = se;
        break;

    case SetEntity2:
		if (entity1!=NULL && RS_Information::isTrimmable(entity1)) {
        	coord2 = mouse;
	        entity2 = se;
		}
        break;

    default:
        break;
    }

    RS_DEBUG->print("RS_ActionModifyBevel::mouseMoveEvent end");
}



void RS_ActionModifyBevel::mouseReleaseEvent(RS_MouseEvent* e) {
    if (RS2::qtToRsButtonState(e->button())==RS2::LeftButton) {
        switch (getStatus()) {
        case SetEntity1:
            if (entity1!=NULL && entity1->isAtomic()) {
                setStatus(SetEntity2);
            }
            break;

        case SetEntity2:
            if (entity2!=NULL && entity2->isAtomic() &&
			    RS_Information::isTrimmable(entity1, entity2)) {
                trigger();
            }
            break;

        default:
            break;
        }
    } else if (RS2::qtToRsButtonState(e->button())==RS2::RightButton) {
        deletePreview();
        init(getStatus()-1);
    }
}



void RS_ActionModifyBevel::commandEvent(RS_CommandEvent* e) {
    RS_String c = e->getCommand().lower();

    if (checkCommand("help", c)) {
        RS_DIALOGFACTORY->commandMessage(msgAvailableCommands()
                                         + getAvailableCommands().join(", "));
        return;
    }

    switch (getStatus()) {
    case SetEntity1:
    case SetEntity2:
        if (checkCommand("length1", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetLength1);
        } else if (checkCommand("length2", c)) {
            deletePreview();
            lastStatus = (Status)getStatus();
            setStatus(SetLength2);
        } else if (checkCommand("trim", c)) {
            //deletePreview();
            //lastStatus = (Status)getStatus();
            //setStatus(SetTrim);
            data.trim = !data.trim;
            RS_DIALOGFACTORY->requestOptions(this, true, true);
        }
        break;

    case SetLength1: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok==true) {
                data.length1 = l;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

    case SetLength2: {
            bool ok;
            double l = RS_Math::eval(c, &ok);
            if (ok==true) {
                data.length2 = l;
            } else {
                RS_DIALOGFACTORY->commandMessage(tr("Not a valid expression"));
            }
            RS_DIALOGFACTORY->requestOptions(this, true, true);
            setStatus(lastStatus);
        }
        break;

        /*case SetTrim: {
        if (checkCommand()) {
        data.trim = true;
    } else if (c==cmdNo.lower() || c==cmdNo2) {
        data.trim = false;
                } else {
                    RS_DIALOGFACTORY->commandMessage(tr("Please enter 'Yes' "
               "or 'No'"));
                }
                RS_DIALOGFACTORY->requestOptions(this, true, true);
                setStatus(lastStatus);
            }
            break;*/

    default:
        break;
    }
}



RS_StringList RS_ActionModifyBevel::getAvailableCommands() {
    RS_StringList cmd;
    switch (getStatus()) {
    case SetEntity1:
    case SetEntity2:
        cmd += command("length1");
        cmd += command("length2");
        cmd += command("trim");
        break;
    default:
        break;
    }
    return cmd;
}



void RS_ActionModifyBevel::showOptions() {
    RS_ActionInterface::showOptions();

    RS_DIALOGFACTORY->requestOptions(this, true);
}



void RS_ActionModifyBevel::hideOptions() {
    RS_ActionInterface::hideOptions();

    RS_DIALOGFACTORY->requestOptions(this, false);
}



void RS_ActionModifyBevel::updateMouseButtonHints() {
    switch (getStatus()) {
    case SetEntity1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select first entity"),
                                            tr("Cancel"));
        break;
    case SetEntity2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Select second entity"),
                                            tr("Back"));
        break;
    case SetLength1:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter length 1:"),
                                            tr("Back"));
        break;
    case SetLength2:
        RS_DIALOGFACTORY->updateMouseWidget(tr("Enter length 2:"),
                                            tr("Back"));
        break;
        /*case SetTrim:
            RS_DIALOGFACTORY->updateMouseWidget(tr("Trim on? (yes/no):"),
                                                "");
            break;*/
    default:
        RS_DIALOGFACTORY->updateMouseWidget("", "");
        break;
    }
}



void RS_ActionModifyBevel::updateMouseCursor() {
    graphicView->setMouseCursor(RS2::CadCursor);
}



void RS_ActionModifyBevel::updateToolBar() {
    RS_DIALOGFACTORY->requestToolBar(RS2::ToolBarModify);
}


// EOF
