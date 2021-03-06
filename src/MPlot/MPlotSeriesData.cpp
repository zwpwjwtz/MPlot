#ifndef __MPlotSeriesData_CPP__
#define __MPlotSeriesData_CPP__

#include "MPlot/MPlotSeriesData.h"

MPlotSeriesDataSignalSource::MPlotSeriesDataSignalSource(MPlotAbstractSeriesData* parent)
	: QObject(0) {
	data_ = parent;
}

MPlotAbstractSeriesData::MPlotAbstractSeriesData()
{
	signalSource_ = new MPlotSeriesDataSignalSource(this);
	cachedDataRectUpdateRequired_ = true;
}

MPlotAbstractSeriesData::~MPlotAbstractSeriesData()
{
	delete signalSource_;
	signalSource_ = 0;
}

#include <QDebug>
#include <QTime>
QRectF MPlotAbstractSeriesData::boundingRect() const {
	if(count() == 0)
		return QRectF();

	if(cachedDataRectUpdateRequired_) {

		qreal minY = searchMinY();
		qreal maxY = searchMaxY();
		qreal minX = searchMinX();
		qreal maxX = searchMaxX();

		cachedDataRect_ = QRectF(minX,
								 minY,
								 qMax(maxX-minX, std::numeric_limits<qreal>::min()),
								 qMax(maxY-minY, std::numeric_limits<qreal>::min()));	// this is to make sure that if we have a single data point (ie: minY=maxY, minX=maxX) there is still a valid boundingRect() for the data.  A QRectF with a width and height of exactly 0 would be indentified as invalid; this makes it valid, without making it much bigger in size.  [min() is the smallest positive number for qreals.]
		cachedDataRectUpdateRequired_ = false;
	}

	return cachedDataRect_;
}

qreal MPlotAbstractSeriesData::searchMinY() const {

	int size = count();
	QVector<qreal> y = QVector<qreal>(size);
	yValues(0, unsigned(size)-1, y.data());

	qreal extreme = y.at(0);

	for (int i = 1; i < size; i++)
		if (y.at(i) < extreme)
			extreme = y.at(i);

	return extreme;
}

qreal MPlotAbstractSeriesData::searchMaxY() const {

	int size = count();
	QVector<qreal> y = QVector<qreal>(size);
	yValues(0, unsigned(size)-1, y.data());

	qreal extreme = y.at(0);

	for (int i = 1; i < size; i++)
		if (y.at(i) > extreme)
			extreme = y.at(i);

	return extreme;
}

qreal MPlotAbstractSeriesData::searchMinX() const {

	int size = count();
	QVector<qreal> x = QVector<qreal>(size);
	xValues(0, unsigned(size)-1, x.data());

	qreal extreme = x.at(0);

	for (int i = 1; i < size; i++)
		if (x.at(i) < extreme)
			extreme = x.at(i);

	return extreme;
}

qreal MPlotAbstractSeriesData::searchMaxX() const {

	int size = count();
	QVector<qreal> x = QVector<qreal>(size);
	xValues(0, unsigned(size)-1, x.data());

	qreal extreme = x.at(0);

	for (int i = 1; i < size; i++)
		if (x.at(i) > extreme)
			extreme = x.at(i);

	return extreme;
}

MPlotRealtimeModel::MPlotRealtimeModel(QObject *parent) :
		QAbstractTableModel(parent), MPlotAbstractSeriesData(), xName_("x"), yName_("y")
{
	// min/max tracking indices are invalid at start (empty model)
	minYIndex_ = maxYIndex_ = minXIndex_ = maxXIndex_ = -1;

	// Axis names: initialized on first line to "x", "y" (Real original... I know.)
}

int MPlotRealtimeModel::rowCount(const QModelIndex & /*parent*/) const {
	return xval_.count();
}

int MPlotRealtimeModel::count() const {
	return xval_.count();
}

int MPlotRealtimeModel::columnCount(const QModelIndex & /*parent*/) const {
	return 2;
}

qreal MPlotRealtimeModel::x(unsigned index) const {
	if(index<(unsigned)xval_.count())
		return xval_.at(index);
	else
		return 0.0;
}

qreal MPlotRealtimeModel::y(unsigned index) const {
	if(index<(unsigned)yval_.count())
		return yval_.at(index);
	else
		return 0.0;
}


QVariant MPlotRealtimeModel::data(const QModelIndex &index, int role) const {

	// Invalid index:
	if(!index.isValid())
		return QVariant();

	// We only answer to Qt::DisplayRole right now
	if(role != Qt::DisplayRole)
		return QVariant();

	// Out of range: (Just checking for too big.  isValid() checked for < 0)
	if(index.row() >= xval_.count())
		return QVariant();

	// Return x-val:
	if(index.column() == 0)
		return xval_.at(index.row());
	// Return y-val:
	if(index.column() == 1)
		return yval_.at(index.row());

	// Anything else:
	return QVariant();
}

QVariant MPlotRealtimeModel::headerData(int section, Qt::Orientation orientation, int role) const {

	if (role != Qt::DisplayRole) return QVariant();

	// Vertical headers:
	if(orientation == Qt::Vertical) {
		return section;
	}

	// Horizontal Headers: (Column labels)
	else {
		if(section == 0)
			return xName_;
		if(section == 1)
			return yName_;
	}
	return QVariant();
}

bool MPlotRealtimeModel::setData(const QModelIndex &index, const QVariant &value, int role) {

	if (index.isValid()  && index.row() < xval_.count() && role == Qt::EditRole) {

		bool conversionOK;
		qreal dval = value.toDouble(&conversionOK);
		if(!conversionOK)
			return false;

		// Setting an x value?
		if(index.column() == 0) {
			minMaxChangeCheckX(dval, index.row());
			emit QAbstractItemModel::dataChanged(index, index);
			emitDataChanged();
			return true;
		}
		// Setting a y value?
		if(index.column() == 1) {
			minMaxChangeCheckY(dval, index.row());
			emit QAbstractItemModel::dataChanged(index, index);
			emitDataChanged();
			return true;
		}
	}
	return false;	// no value set
}

// This allows editing of values within range (for ex: in a QTableView)
Qt::ItemFlags MPlotRealtimeModel::flags(const QModelIndex &index) const {

	Qt::ItemFlags flags;
	if (index.isValid() && index.row() < xval_.count() && index.column()<2)
		flags = Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled;
	return flags;
}

// This allows you to add data points at the beginning:
void MPlotRealtimeModel::insertPointFront(qreal x, qreal y) {
	beginInsertRows(QModelIndex(), 0, 0);

	xval_.prepend(x);
	yval_.prepend(y);

	// Update the max/min index trackers:
	minXIndex_++;
	minYIndex_++;
	maxXIndex_++;
	maxYIndex_++;

	// Check if this guy is a new min or max:
	minMaxAddCheck(x, y, 0);

	endInsertRows();

	// Signal a full-plot update
	emitDataChanged();
}

// This allows you to add data points at the end:
void MPlotRealtimeModel::insertPointBack(qreal x, qreal y) {
	beginInsertRows(QModelIndex(), xval_.count(), xval_.count());

	xval_.append(x);
	yval_.append(y);

	minMaxAddCheck(x, y, xval_.count()-1);

	endInsertRows();
	// Signal a full-plot update
	emitDataChanged();
}

// Remove a point at the front (Returns true if successful).
bool MPlotRealtimeModel::removePointFront() {
	if(xval_.isEmpty())
		return false;

	beginRemoveRows(QModelIndex(), 0, 0);

	xval_.takeFirst();
	yval_.takeFirst();

	// Update the max/min index trackers:
	minXIndex_--;
	minYIndex_--;
	maxXIndex_--;
	maxYIndex_--;

	// Did we remove the current max/min?
	if(minYIndex_ == -1)
		minYIndex_ = searchMinIndex(yval_);
	if(maxYIndex_ == -1)
		maxYIndex_ = searchMaxIndex(yval_);
	if(minXIndex_ == -1)
		minXIndex_ = searchMinIndex(xval_);
	if(maxXIndex_ == -1)
		maxXIndex_ = searchMaxIndex(xval_);


	endRemoveRows();

	// Signal a full-plot update
	emitDataChanged();
	return true;
}

// Remove a point at the back (returns true if successful)
bool MPlotRealtimeModel::removePointBack() {
	if(xval_.isEmpty())
		return false;

	beginRemoveRows(QModelIndex(), xval_.count()-1, xval_.count()-1);

	xval_.takeLast();
	yval_.takeLast();

	// Did we remove the current max/min?
	int oldIndexToCheck = xval_.count();
	if(minYIndex_ == oldIndexToCheck)
		minYIndex_ = searchMinIndex(yval_);
	if(maxYIndex_ == oldIndexToCheck)
		maxYIndex_ = searchMaxIndex(yval_);
	if(minXIndex_ == oldIndexToCheck)
		minXIndex_ = searchMinIndex(xval_);
	if(maxXIndex_ == oldIndexToCheck)
		maxXIndex_ = searchMaxIndex(xval_);

	endRemoveRows();

	// Signal a full-plot update
	emitDataChanged();
	return true;
}

QRectF MPlotRealtimeModel::boundingRect() const {
	if(xval_.isEmpty() || yval_.isEmpty())
		return QRectF();	// No data... return an invalid QRectF

	return QRectF(minX(), minY(), maxX()-minX(), maxY()-minY());

}



// Helper functions:
// Check if an added point @ index is the new min. or max record holder:
// Must call this AFTER adding both x and y to the xval_ and y_val lists.
void MPlotRealtimeModel::minMaxAddCheck(qreal x, qreal y, int index) {

	// in case of previously-empty list (min_Index_ initialized to -1)
	if(xval_.count() == 1) {
		minYIndex_ = index;
		maxYIndex_ = index;
		minXIndex_ = index;
		maxXIndex_ = index;
		return;
	}

	// New x max found:
	if(x > xval_.at(maxXIndex_))
		maxXIndex_ = index;
	// New x min found:
	if(x < xval_.at(minXIndex_))
		minXIndex_ = index;
	// New y max found:
	if(y > yval_.at(maxYIndex_))
		maxYIndex_ = index;
	if(y < yval_.at(minYIndex_))
		minYIndex_ = index;

}

// Check if a point modified at index causes us to lose our record holders, or is a new record holder.
// Inserts the point (modifies the data array).
void MPlotRealtimeModel::minMaxChangeCheckX(qreal newVal, int index) {

	qreal oldVal = xval_.at(index);
	xval_[index] = newVal;

	// Maybe not the max anymore... Need to find the new one:
	if(index == maxXIndex_ && newVal < oldVal)
		maxXIndex_ = searchMaxIndex(xval_);

	// Maybe not the min anymore... Need to find the new one:
	if(index == minXIndex_ && newVal > oldVal)
		minXIndex_ = searchMinIndex(xval_);

	if(newVal > xval_.at(maxXIndex_))
		maxXIndex_ = index;

	if(newVal < xval_.at(minXIndex_))
		minXIndex_ = index;


}
void MPlotRealtimeModel::minMaxChangeCheckY(qreal newVal, int index) {

	qreal oldVal = yval_.at(index);
	yval_[index] = newVal;

	// Maybe not the max anymore... Need to find the new one:
	if(index == maxYIndex_ && newVal < oldVal)
		maxYIndex_ = searchMaxIndex(yval_);

	// Maybe not the min anymore... Need to find the new one:
	if(index == minYIndex_ && newVal > oldVal)
		minYIndex_ = searchMinIndex(yval_);

	if(newVal > yval_.at(maxYIndex_))
		maxYIndex_ = index;

	if(newVal < yval_.at(minYIndex_))
		minYIndex_ = index;


}

int MPlotRealtimeModel::searchMaxIndex(const QList<qreal>& list) {
	if(list.isEmpty())
		return -1;

	int mi = 0;	// max index
	for(int i=1; i<list.count(); i++)
		if(list.at(i) > list.at(mi))
			mi = i;

	return mi;
}

int MPlotRealtimeModel::searchMinIndex(const QList<qreal>& list) {
	if(list.isEmpty())
		return -1;

	int mi = 0;	// min index
	for(int i=1; i<list.count(); i++)
		if(list.at(i) < list.at(mi))
			mi = i;

	return mi;
}

// Warning: only call these if the list is not empty:
qreal MPlotRealtimeModel::minY() const {
	return yval_.at(minYIndex_);
}

qreal MPlotRealtimeModel::maxY() const {
	return yval_.at(maxYIndex_);
}

qreal MPlotRealtimeModel::minX() const {
	return xval_.at(minXIndex_);
}

qreal MPlotRealtimeModel::maxX() const {
	return xval_.at(maxXIndex_);
}

bool MPlotVectorSeriesData::setValues(const QVector<qreal> &xValues, const QVector<qreal> &yValues)
{
	if(xValues.count() != yValues.count())
		return false;

	xValues_ = xValues;
	yValues_ = yValues;
	emitDataChanged();
	return true;
}

bool MPlotVectorSeriesData::setXValue(int index, qreal xValue)
{
	if((unsigned)index >= (unsigned)xValues_.count())
		return false;

	xValues_[index] = xValue;
	emitDataChanged();
	return true;
}

bool MPlotVectorSeriesData::setYValue(int index, qreal yValue)
{
	if((unsigned)index >= (unsigned)yValues_.count())
		return false;

	yValues_[index] = yValue;
	emitDataChanged();
	return true;
}

MPlotVectorSeriesData::MPlotVectorSeriesData()
	: MPlotAbstractSeriesData()
{
}

void MPlotRealtimeModel::xValues(unsigned indexStart, unsigned indexEnd, qreal *outputValues) const
{
	for(unsigned i=indexStart; i<=indexEnd; ++i)
		*(outputValues++) = xval_.at(i);
}

void MPlotRealtimeModel::yValues(unsigned indexStart, unsigned indexEnd, qreal *outputValues) const
{
	for(unsigned i=indexStart; i<=indexEnd; ++i)
		*(outputValues++) = yval_.at(i);
}


#endif

