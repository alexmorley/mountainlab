/******************************************************
** See the accompanying README and LICENSE files
** Author(s): Jeremy Magland
** Created: 2/22/2017
*******************************************************/
#ifndef P_EXTRACT_CLIPS_H
#define P_EXTRACT_CLIPS_H

#include <QVariantMap>

bool p_extract_clips(QString timeseries, QString event_times, QString clips_out, const QVariantMap& params);

#endif // P_EXTRACT_CLIPS_H
