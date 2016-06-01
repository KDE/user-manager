/**
 *  Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>
 *  Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>
 *  Copyright 2015 Kai Uwe Broulik <kde@privat.broulik.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *
 */

#ifndef AVATARGALLERY_H
#define AVATARGALLERY_H

#include <QDialog>
#include <QUrl>

#include "ui_avatargallery.h"

class AvatarGallery : public QDialog
{
    Q_OBJECT

public:
    explicit AvatarGallery(QWidget *parent = nullptr);
    virtual ~AvatarGallery() = default;

    QUrl url() const;

private:
    Ui::faceDlg ui;

};

#endif // AVATARGALLERY_H

