#include "window.h"

Window::Window()
{
	image = new QLabel;

	// plot to the left of button and thermometer
	hLayout = new QHBoxLayout();
	hLayout->addWidget(image);

	setLayout(hLayout);
}

void Window::updateImage(const cv::Mat &mat) {
	const QImage frame(mat.data, mat.cols, mat.rows, mat.step,
			   QImage::Format_BGR888);
	image->setPixmap(QPixmap::fromImage(frame));
	update();
}
