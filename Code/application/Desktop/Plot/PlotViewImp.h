/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef PLOTVIEWIMP_H
#define PLOTVIEWIMP_H

#include <QtWidgets/QAction>
#include <QtWidgets/QMenu>

#include "PolygonPlotObjectAdapter.h"
#include "OrthographicViewImp.h"
#include "LocatorAdapter.h"

#include <list>
#include <string>
#include <unordered_set>

class AnnotationLayer;
class AnnotationLayerAdapter;
class PlotObject;
class PointSet;

class PlotViewImp : public OrthographicViewImp
{
   Q_OBJECT

public:
   PlotViewImp(const std::string& id, const std::string& viewName, QGLContext* drawContext = 0, QWidget* parent = 0);
   ~PlotViewImp();

   using OrthographicViewImp::setIcon;
   using OrthographicViewImp::setName;
   bool canRename() const;

   const std::string& getObjectType() const;
   bool isKindOf(const std::string& className) const;

   static bool isKindOfView(const std::string& className);
   static void getViewTypes(std::vector<std::string>& classList);

   ViewType getViewType() const;

   virtual PlotType getPlotType() const = 0;

   // Objects
   PlotObject* createObject(const PlotObjectType& objectType, bool bPrimary, bool bQuiet = false);
   PlotObject* addObject(const PlotObjectType& objectType, bool bPrimary, bool bQuiet = false);
   bool insertObject(PlotObject* pObject);
   bool insertObjects(const std::list<PlotObject*>& objects);
   std::list<PlotObject*> getObjects() const;
   std::list<PlotObject*> getObjects(const PlotObjectType& objectType) const;
   std::list<PlotObject*> getObjectsAt(const QPoint& point) const;
   bool containsObject(PlotObject* pObject) const;
   unsigned int getNumObjects() const;
   bool deleteObject(PlotObject* pObject);
   bool deleteObjects(std::unordered_set<PlotObject*>& objects);

   bool moveObjectToFront(PlotObject* pObject);
   bool moveObjectToBack(PlotObject* pObject);

   void selectObjects(const std::list<PlotObject*>& objects, bool bSelect);
   void selectObjects(const QRegion& selectionRegion, bool bSelect);
   void selectObjects(const QRegion& selectionRegion, const PointSet* pPointSet, bool bSelect);
   void selectObjects(const QPoint& point, const PointSet* pPointSet, bool bSelect);
   std::list<PlotObject*> getSelectedObjects(bool filterVisible) const;
   unsigned int getNumSelectedObjects(bool filterVisible) const;

   void setSelectionMode(PlotSelectionModeType mode);
   PlotSelectionModeType getSelectionMode() const;
   void setSelectionDisplayMode(PointSelectionDisplayType mode);
   PointSelectionDisplayType getSelectionDisplayMode() const;
   void setEnableShading( bool shading );
   bool isShadingEnabled() const;
   void setExtentsMargin(double marginFactor);
   double getExtentsMargin() const;
   void getExtents(double& dMinX, double& dMinY, double& dMaxX, double& dMaxY) const;

   // Annotation
   AnnotationLayer* getAnnotationLayer() const;

   // Mouse locator
   Locator* getMouseLocator();
   const Locator* getMouseLocator() const;

   // Coordinate transformations
   virtual void translateWorldToData(double worldX, double worldY, double& dataX, double& dataY) const;
   virtual void translateDataToWorld(double dataX, double dataY, double& worldX, double& worldY) const;
   virtual void translateScreenToData(double screenX, double screenY, double& dataX, double& dataY) const;
   virtual void translateDataToScreen(double dataX, double dataY, double& screenX, double& screenY) const;

   void draw();
   GLuint getDisplayListIndex() const;

   bool toXml(XMLWriter* pXml) const;
   bool fromXml(DOMNode* pDocument, unsigned int version);

public slots:
   bool selectObject(PlotObject* pObject, bool bSelect);
   void selectObjects(bool bSelect);
   void deleteSelectedObjects(bool filterVisible);
   void clear();

signals:
   void objectAdded(PlotObject* pObject);
   void objectDeleted(PlotObject* pObject);
   void objectSelected(PlotObject* pObject, bool bSelected);

protected:
   PlotViewImp& operator=(const PlotViewImp& plotView);
   using OrthographicViewImp::setMouseMode;
   void keyPressEvent(QKeyEvent* pEvent);
   void mousePressEvent(QMouseEvent* e);
   void mouseMoveEvent(QMouseEvent* e);
   void mouseReleaseEvent(QMouseEvent* e);
   void mouseDoubleClickEvent(QMouseEvent* pEvent);
   void resizeEvent(QResizeEvent* pEvent);
   void drawContents();
   virtual void drawGridlines() = 0;
   void initializeDisplayList();
   bool insertObjectSafe(PlotObject* pObject);
   void selectObjectsSafe(const std::list<PlotObject*>& objects, bool select);
   bool selectObjectSafe(PlotObject* pObject, bool select);

protected slots:
   void setMouseMode(QAction* pAction);
   void addMouseModeAction(const MouseMode* pMouseMode);
   void removeMouseModeAction(const MouseMode* pMouseMode);
   void enableMouseModeAction(const MouseMode* pMouseMode, bool bEnable);
   void updateMouseModeAction(const MouseMode* pMouseMode);
   void updateExtents(bool deep=false);
   void updateAnnotationObjects();

private:
   PlotViewImp(const PlotViewImp& rhs);

   QMenu* mpMouseModeMenu;
   QActionGroup* mpMouseModeGroup;
   QAction* mpObjectSelectAction;
   QAction* mpPanAction;
   QAction* mpZoomAction;
   QAction* mpLocateAction;
   QAction* mpAnnotateAction;

   std::list<PlotObject*> mObjects;

   AnnotationLayerAdapter* mpAnnotationLayer;

   LocatorAdapter mMouseLocator;
   PolygonPlotObjectAdapter mSelectionArea;
   bool mDisplayListInitialized;
   GLuint mDisplayListIndex;          // openGL display list index

   PlotSelectionModeType mSelectionMode;
   PointSelectionDisplayType mSelectionDisplayMode;
   bool mEnableShading;
   double mExtentsMargin;
   bool mExtentsDirty;

private:
   QRegion getObjectRegion(const PlotObject* pObject) const;
   void calculateExtents();
};

#define PLOTVIEWADAPTEREXTENSION_CLASSES \
   ORTHOGRAPHICVIEWADAPTEREXTENSION_CLASSES

#define PLOTVIEWADAPTER_METHODS(impClass) \
   ORTHOGRAPHICVIEWADAPTER_METHODS(impClass) \
   PlotType getPlotType() const \
   { \
      return impClass::getPlotType(); \
   } \
   PlotObject* createObject(const PlotObjectType& objectType, bool bPrimary) \
   { \
      return impClass::createObject(objectType, bPrimary, false); \
   } \
   PlotObject* createQuietObject(const PlotObjectType& objectType, bool bPrimary) \
   { \
      return impClass::createObject(objectType, bPrimary, true); \
   } \
   PlotObject* addObject(const PlotObjectType& objectType, bool bPrimary) \
   { \
      return impClass::addObject(objectType, bPrimary, false); \
   } \
   PlotObject* addQuietObject(const PlotObjectType& objectType, bool bPrimary) \
   { \
      return impClass::addObject(objectType, bPrimary, true); \
   } \
   bool insertObject(PlotObject* pObject) \
   { \
      return impClass::insertObject(pObject); \
   } \
   bool insertObjects(const std::list<PlotObject*>& objects) \
   { \
      return impClass::insertObjects(objects); \
   } \
   void getObjects(std::list<PlotObject*>& objects) const \
   { \
      objects = impClass::getObjects(); \
   } \
   void getObjects(const PlotObjectType& objectType, std::list<PlotObject*>& objects) const \
   { \
      objects = impClass::getObjects(objectType); \
   } \
   bool containsObject(PlotObject* pObject) const \
   { \
      return impClass::containsObject(pObject); \
   } \
   unsigned int getNumObjects() const \
   { \
      return impClass::getNumObjects(); \
   } \
   bool deleteObject(PlotObject* pObject) \
   { \
      return impClass::deleteObject(pObject); \
   } \
   bool deleteObjects(std::unordered_set<PlotObject*>& objects) \
   { \
      return impClass::deleteObjects(objects); \
   } \
   void clear() \
   { \
      return impClass::clear(); \
   } \
   bool moveObjectToFront(PlotObject* pObject) \
   { \
      return impClass::moveObjectToFront(pObject); \
   } \
   bool moveObjectToBack(PlotObject* pObject) \
   { \
      return impClass::moveObjectToBack(pObject); \
   } \
   bool selectObject(PlotObject* pObject, bool bSelect) \
   { \
      return impClass::selectObject(pObject, bSelect); \
   } \
   void selectObjects(const std::list<PlotObject*>& objects, bool bSelect) \
   { \
      return impClass::selectObjects(objects, bSelect); \
   } \
   void selectObjects(bool bSelect) \
   { \
      return impClass::selectObjects(bSelect); \
   } \
   void getSelectedObjects(std::list<PlotObject*>& selectedObjects, bool filterVisible) const \
   { \
      selectedObjects = impClass::getSelectedObjects(filterVisible); \
   } \
   unsigned int getNumSelectedObjects(bool filterVisible) const \
   { \
      return impClass::getNumSelectedObjects(filterVisible); \
   } \
   void deleteSelectedObjects(bool filterVisible) \
   { \
      return impClass::deleteSelectedObjects(filterVisible); \
   } \
   void setSelectionMode(PlotSelectionModeType mode) \
   { \
      return impClass::setSelectionMode(mode); \
   } \
   PlotSelectionModeType getSelectionMode() const \
   { \
      return impClass::getSelectionMode(); \
   } \
   void setSelectionDisplayMode(PointSelectionDisplayType mode) \
   { \
      return impClass::setSelectionDisplayMode(mode); \
   } \
   PointSelectionDisplayType getSelectionDisplayMode() const \
   { \
      return impClass::getSelectionDisplayMode(); \
   } \
   void setEnableShading(bool shading) \
   { \
      return impClass::setEnableShading(shading); \
   } \
   bool isShadingEnabled() const \
   { \
      return impClass::isShadingEnabled(); \
   } \
   void setExtentsMargin(double marginFactor) \
   { \
      impClass::setExtentsMargin(marginFactor); \
   } \
   double getExtentsMargin() const \
   { \
      return impClass::getExtentsMargin(); \
   } \
   AnnotationLayer* getAnnotationLayer() const \
   { \
      return impClass::getAnnotationLayer(); \
   } \
   Locator* getMouseLocator() \
   { \
      return impClass::getMouseLocator(); \
   } \
   const Locator* getMouseLocator() const \
   { \
      return impClass::getMouseLocator(); \
   } \
   void translateWorldToData(double worldX, double worldY, double& dataX, double& dataY) const \
   { \
      return impClass::translateWorldToData(worldX, worldY, dataX, dataY); \
   } \
   void translateDataToWorld(double dataX, double dataY, double &worldX, double &worldY) const \
   { \
      return impClass::translateDataToWorld(dataX, dataY, worldX, worldY); \
   } \
   void translateScreenToData(double screenX, double screenY, double& dataX, double& dataY) const \
   { \
      return impClass::translateScreenToData(screenX, screenY, dataX, dataY); \
   } \
   void translateDataToScreen(double dataX, double dataY, double& screenX, double& screenY) const \
   { \
      return impClass::translateDataToScreen(dataX, dataY, screenX, screenY); \
   }

#endif
