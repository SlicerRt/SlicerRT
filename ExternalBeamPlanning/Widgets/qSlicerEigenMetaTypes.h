#ifndef __qSlicerEigenMetaTypes_h
#define __qSlicerEigenMetaTypes_h

// Qt includes
#include <QMetaType>

// ITK includes
#include <itkeigen/Eigen/Dense>

// Eigen types used in Q_INVOKABLE methods. With Qt6, moc instantiates the metatype of
// Q_INVOKABLE argument types, therefore they must be declared once, before any class using them.
Q_DECLARE_METATYPE(Eigen::VectorXd)

#endif
