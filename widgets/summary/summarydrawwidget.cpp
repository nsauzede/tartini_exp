/***************************************************************************
                          summarydrawwidget.cpp  -  description
                             -------------------
    begin                : Fri Dec 10 2004
    copyright            : (C) 2004-2005 by Philip McLeod
    email                : pmcleod@cs.otago.ac.nz
    copyright            : (C) 2016 by Julien Thevenon
    email                : julien_thevenon at yahoo.fr
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   Please read LICENSE.txt for details.
 ***************************************************************************/

#include "summarydrawwidget.h"
#include "gdata.h"
#include "channel.h"

#include <QThread>

#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>

//------------------------------------------------------------------------------
SummaryDrawWidget::SummaryDrawWidget(QWidget *p_parent)
: DrawWidget(p_parent)
{
    m_scaler = 1.0;

    //make the widget get updated when the view changes
    connect(&(gdata->getView()), SIGNAL(onFastUpdate(double)), this, SLOT(update()));
}

//------------------------------------------------------------------------------
SummaryDrawWidget::~SummaryDrawWidget(void)
{
}

//------------------------------------------------------------------------------
void SummaryDrawWidget::paintEvent(QPaintEvent *)
{
    Channel *l_channel;

    View & l_view = gdata->getView();
    if(gdata->totalTime() < 0)
    {
        return;
    }

    double l_time_ratio = double(width()) / gdata->totalTime();
    double l_pitch_ratio = double(height()) / (gdata->topPitch() / m_scaler);

    beginDrawing();

    //draw all the channels
    for(int l_j = 0; l_j < (int)gdata->getChannelsSize(); l_j++)
    {
        l_channel = gdata->getChannelAt(l_j);
        if(!l_channel->isVisible())
        {
            continue;
        }
        drawChannel(*this, l_channel, get_painter(), gdata->leftTime(), l_view.currentTime(), (gdata->totalTime() / (double) width()), 0.0f, (double) gdata->topPitch() / (double) height(), DRAW_VIEW_SUMMARY);
    }

    //draw the view rectangle
    get_painter().setPen(QPen(colorGroup().highlight(), 1));
    get_painter().drawRect(int((gdata->leftTime() + l_view.viewLeft()) * l_time_ratio)
                          ,height() - 1 - int((l_view.viewTop()) * l_pitch_ratio)
                          ,int(l_view.viewWidth() * l_time_ratio)
                          ,int(l_view.viewHeight() * l_pitch_ratio)
                          );

    //draw the current time line
    get_painter().setPen(QPen(colorGroup().foreground(), 1));
    get_painter().drawLine(int((gdata->leftTime() + l_view.currentTime()) * l_time_ratio)
                          ,0
                          ,int((gdata->leftTime() + l_view.currentTime()) * l_time_ratio)
                          ,height() - 1
                          );

    endDrawing();
}

//------------------------------------------------------------------------------
void SummaryDrawWidget::mousePressEvent(QMouseEvent * p_event)
{
    View & l_view = gdata->getView();
    double l_time_ratio = double(width()) / gdata->totalTime(); // px per second
    double l_note_ratio = double(height()) / (100.0 / m_scaler);
    m_click_mode = 0;
    m_mouse_down = true;
    m_mouse_X = p_event->x();
    m_mouse_Y = p_event->y();

    // Work out where the user clicked
    m_click_time = p_event->x() / l_time_ratio + gdata->leftTime();
    m_click_note = (height() - 1 - p_event->y()) / l_note_ratio;
    if(m_click_time > l_view.viewLeft() && m_click_time < l_view.viewRight() &&
       m_click_note > l_view.viewBottom() && m_click_note < l_view.viewTop())
    {
        // If they're in the view, they're dragging it.
        m_click_current_time_diff = l_view.currentTime() - m_click_time;
        m_click_view_bottom_diff = l_view.viewBottom() - m_click_note;
        m_click_mode = 1;
        m_down_scaler = m_scaler;
    }
    else
    {
        // Otherwise, move the view to where they clicked.
        l_view.setCurrentTime(m_click_time);
        l_view.setViewBottom(int(myround(m_click_note - (l_view.viewHeight() / 2))));
        m_click_mode = 1;
    }
}

//------------------------------------------------------------------------------
void SummaryDrawWidget::mouseMoveEvent(QMouseEvent * p_event)
{
    if(m_mouse_down && m_click_mode == 1)
    {
        View & l_view = gdata->getView();
        double l_time_ratio = double(width()) / gdata->totalTime();
        double l_note_ratio = double(height()) / (100.0 / m_scaler);

        // We should only change the current time if the file is stopped
        if(gdata->getRunning() == STREAM_STOP)
        {
            l_view.setCurrentTime(p_event->x() / l_time_ratio - gdata->leftTime() + m_click_current_time_diff);
        }
        double l_new_bottom = (height() - 1 - p_event->y()) / l_note_ratio + m_click_view_bottom_diff;
        if(l_new_bottom < 0)
        {
            l_new_bottom = 0;
        }
        if((l_new_bottom + l_view.viewHeight()) > gdata->topPitch())
        {
            l_new_bottom = gdata->topPitch() - l_view.viewHeight();
        }
        l_view.setViewBottom(l_new_bottom);
    }
}

//------------------------------------------------------------------------------
void SummaryDrawWidget::mouseReleaseEvent(QMouseEvent *)
{
    m_mouse_down = false;
}

//------------------------------------------------------------------------------
void SummaryDrawWidget::wheelEvent(QWheelEvent * p_event)
{
    p_event->accept();

    if(p_event->state() == Qt::AltModifier)
    {
        int l_hor_scale = toInt(gdata->totalTime() / width() * (p_event->delta() / 10));
        gdata->getView().setCurrentTime(gdata->getView().currentTime() + l_hor_scale);
    }
    else
    {
        int l_vert_scale = toInt(gdata->topPitch() / height() * (p_event->delta() / 10));
        gdata->getView().setViewBottom(gdata->getView().viewBottom() + l_vert_scale);
    }
}

// EOF
