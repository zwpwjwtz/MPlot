#ifndef __MPlotWidget_H__
#define __MPlotWidget_H__

#include <QGraphicsView>
#include <QResizeEvent>
#include "MPlot.h"


// TODO: test performance of:
// setItemIndexMethod(NoIndex);
// makes a big difference if drawing plots using many separate QGraphicsItem elements (for ex: separate QGraphicsLineItems for each line element in a series)

/// This class holds the scene and view for MPlot.  Everything inside the plot will be part of the MPlot scene and viewed through this class.
class MPlotSceneAndView : public QGraphicsView {
	Q_OBJECT

public:
        /// Constructor.  Builds the scene and view that the plot will reside in.
	MPlotSceneAndView(QWidget* parent = 0);

        /// Setter to enable/disable anti-aliasing.
	void enableAntiAliasing(bool antiAliasingOn = true);
        /// Destructor.
	virtual ~MPlotSceneAndView();

protected:

        /// On resize events: keep the scene the same size as the view, and make the view look at this part of the scene.
	virtual void resizeEvent ( QResizeEvent * event );

};

/// This class is the widget that holds the plot.  It extends the standard scene and view requirements of basic visualization for the plot to be visualized properly.
class MPlotWidget : public MPlotSceneAndView {
	Q_OBJECT

public:
        /// Constructor.  Builds the scene and view, and sets up everything to make viewing a plot possible.
	MPlotWidget(QWidget* parent = 0);
        /// Destructor.
	virtual ~MPlotWidget();

	/// Sets the plot attached to this widget. to remove a plot, pass \c plot = 0.
	void setPlot(MPlot* plot);

        /// Returns a pointer to the plot that this widget is painting.
	MPlot* plot();

protected:

        // On resize events: notify the plot to resize it, and fill the viewport with the canvas.
        virtual void resizeEvent ( QResizeEvent * event );

	// Member variables:
	MPlot* plot_;
};

#endif
