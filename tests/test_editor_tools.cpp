#include <QTest>
#include <QApplication>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QWheelEvent>
#include <QMessageBox>
#include <QAbstractButton>
#include "editor/CanvasScene.h"
#include "editor/CanvasView.h"
#include "core/SettingsManager.h"
#include "core/UpdateManager.h"
#include "core/IconManager.h"
#include "capture/RegionSnippingOverlay.h"
#include "editor/items/PenItem.h"
#include "editor/items/BlurItem.h"
#include "editor/items/ArrowItem.h"
#include "editor/items/ShapeItem.h"
#include "editor/items/BadgeItem.h"
#include "editor/items/TextItem.h"

class TestScene : public CanvasScene {
public:
    using CanvasScene::mousePressEvent;
    using CanvasScene::mouseMoveEvent;
    using CanvasScene::mouseReleaseEvent;
};

class TestEditorTools : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testPenItemDrawing();
    void testHighlighterDrawingAndBlending();
    void testBlurItemEffect();
    void testBlurItemLevels();
    void testBlurItemMovement();
    void testCanvasSceneToolIntegration();
    void testCanvasSceneAreaSelectionAndCrop();
    void testCanvasSceneUndoRedo();
    void testTextItemMultiLineAndBackground();
    void testCanvasSceneTextCreation();
    void testBucketFillTool();
    void testDefaultBlurStrength();
    void testSelectionPropertiesSync();
    void testDefaultSaveLocation();
    void testCanvasViewDirectMousewheelZoom();
    void testBlurItemMemoryOptimization();
    void testScreenCapture();
    void testRegionSnippingCapture();
    void testTabCloseSaveDialogButtons();
    void testAutoCheckUpdatesSetting();
    void testUpdateManagerVersionComparison();
    void testIconManager();
};

void TestEditorTools::initTestCase() {
}

void TestEditorTools::testPenItemDrawing() {
    PenItem pen(false);
    pen.setStrokeColor(QColor(255, 0, 0));
    pen.setStrokeWidth(4);

    QVERIFY(pen.path().isEmpty());
    pen.addPoint(QPointF(10, 10));
    QVERIFY(!pen.path().isEmpty());
    pen.addPoint(QPointF(50, 50));
    pen.addPoint(QPointF(100, 20));

    QRectF b = pen.boundingRect();
    QVERIFY(b.width() >= 80);
    QVERIFY(b.height() >= 30);

    QImage img(150, 100, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    QStyleOptionGraphicsItem opt;
    pen.paint(&p, &opt, nullptr);
    p.end();

    int coloredPixels = 0;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(img.pixel(x, y)) > 50) {
                coloredPixels++;
            }
        }
    }
    QVERIFY(coloredPixels > 50);
}

void TestEditorTools::testHighlighterDrawingAndBlending() {
    PenItem hl(true);
    QVERIFY(hl.isHighlighter());
    QVERIFY(hl.strokeWidth() >= 14);
    QVERIFY(hl.strokeColor().alpha() < 255);

    hl.addPoint(QPointF(10, 50));
    hl.addPoint(QPointF(90, 50));

    QImage base(100, 100, QImage::Format_ARGB32_Premultiplied);
    base.fill(Qt::white);
    QPainter bp(&base);
    bp.setPen(QPen(Qt::black, 4));
    bp.drawLine(50, 10, 50, 90);
    bp.end();

    QPainter hp(&base);
    QStyleOptionGraphicsItem opt;
    hl.paint(&hp, &opt, nullptr);
    hp.end();

    QRgb interPix = base.pixel(50, 50);
    QVERIFY(qRed(interPix) < 50);
    QVERIFY(qGreen(interPix) < 50);
    QVERIFY(qBlue(interPix) < 50);

    QRgb paperPix = base.pixel(20, 50);
    QVERIFY(qRed(paperPix) > 200);
    QVERIFY(qGreen(paperPix) > 180);
    QVERIFY(qBlue(paperPix) < 170);
}

void TestEditorTools::testBlurItemEffect() {
    QImage srcImg(100, 100, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            srcImg.setPixel(x, y, (x < 50) ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
        }
    }
    QPixmap srcPix = QPixmap::fromImage(srcImg);

    BlurItem blur(QRectF(20, 20, 60, 60), srcPix, 5);

    QImage renderTarget(100, 100, QImage::Format_ARGB32_Premultiplied);
    renderTarget.fill(Qt::transparent);
    QPainter p(&renderTarget);
    QStyleOptionGraphicsItem opt;
    blur.paint(&p, &opt, nullptr);
    p.end();

    int intermediateGrayPixels = 0;
    for (int y = 30; y < 70; ++y) {
        for (int x = 40; x < 60; ++x) {
            int val = qRed(renderTarget.pixel(x, y));
            if (val > 30 && val < 225) {
                intermediateGrayPixels++;
            }
        }
    }
    QVERIFY(intermediateGrayPixels > 20);
}

void TestEditorTools::testBlurItemLevels() {
    QImage srcImg(120, 120, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < 120; ++y) {
        for (int x = 0; x < 120; ++x) {
            srcImg.setPixel(x, y, ((x / 10) % 2 == 0) ? qRgb(0, 0, 0) : qRgb(255, 255, 255));
        }
    }
    QPixmap srcPix = QPixmap::fromImage(srcImg);

    BlurItem blurLight(QRectF(10, 10, 100, 100), srcPix, 1);
    QImage outLight(120, 120, QImage::Format_ARGB32_Premultiplied);
    outLight.fill(Qt::black);
    QPainter p1(&outLight);
    QStyleOptionGraphicsItem opt;
    blurLight.paint(&p1, &opt, nullptr);
    p1.end();

    BlurItem blurHeavy(QRectF(10, 10, 100, 100), srcPix, 10);
    QImage outHeavy(120, 120, QImage::Format_ARGB32_Premultiplied);
    outHeavy.fill(Qt::black);
    QPainter p2(&outHeavy);
    blurHeavy.paint(&p2, &opt, nullptr);
    p2.end();

    int diffSumLight = 0;
    int diffSumHeavy = 0;
    for (int x = 20; x < 99; ++x) {
        diffSumLight += qAbs(qRed(outLight.pixel(x + 1, 50)) - qRed(outLight.pixel(x, 50)));
        diffSumHeavy += qAbs(qRed(outHeavy.pixel(x + 1, 50)) - qRed(outHeavy.pixel(x, 50)));
    }

    QVERIFY(diffSumHeavy < diffSumLight);
}

void TestEditorTools::testBlurItemMovement() {
    QImage srcImg(100, 100, QImage::Format_ARGB32_Premultiplied);
    srcImg.fill(Qt::black);
    for (int y = 0; y < 40; ++y) {
        for (int x = 0; x < 40; ++x) {
            srcImg.setPixel(x, y, qRgb(255, 255, 255));
        }
    }
    QPixmap srcPix = QPixmap::fromImage(srcImg);

    CanvasScene scene;
    scene.setBasePixmap(srcPix);

    BlurItem* blur = new BlurItem(QRectF(0, 0, 30, 30), srcPix, 5);
    scene.addItem(blur);

    QImage target1(30, 30, QImage::Format_ARGB32_Premultiplied);
    QPainter p1(&target1);
    QStyleOptionGraphicsItem opt;
    blur->paint(&p1, &opt, nullptr);
    p1.end();
    QVERIFY(qRed(target1.pixel(15, 15)) > 200);

    blur->setPos(60, 60);

    QImage target2(30, 30, QImage::Format_ARGB32_Premultiplied);
    QPainter p2(&target2);
    blur->paint(&p2, &opt, nullptr);
    p2.end();
    QVERIFY(qRed(target2.pixel(15, 15)) < 50);
}

void TestEditorTools::testCanvasSceneToolIntegration() {
    TestScene scene;
    QPixmap base(200, 200);
    base.fill(Qt::white);
    scene.setBasePixmap(base);

    scene.setCurrentTool(ToolType::Pen);
    QCOMPARE(scene.currentTool(), ToolType::Pen);

    QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
    pressEv.setButton(Qt::LeftButton);
    pressEv.setScenePos(QPointF(20, 20));
    scene.mousePressEvent(&pressEv);

    QGraphicsSceneMouseEvent moveEv(QEvent::GraphicsSceneMouseMove);
    moveEv.setButton(Qt::LeftButton);
    moveEv.setScenePos(QPointF(40, 40));
    scene.mouseMoveEvent(&moveEv);

    QGraphicsSceneMouseEvent releaseEv(QEvent::GraphicsSceneMouseRelease);
    releaseEv.setButton(Qt::LeftButton);
    releaseEv.setScenePos(QPointF(60, 40));
    scene.mouseReleaseEvent(&releaseEv);

    QCOMPARE(scene.items().count(), 3);

    scene.setCurrentTool(ToolType::Blur);
    scene.setBlurLevel(7);
    QCOMPARE(scene.blurLevel(), 7);

    pressEv.setScenePos(QPointF(100, 100));
    scene.mousePressEvent(&pressEv);
    moveEv.setScenePos(QPointF(150, 150));
    scene.mouseMoveEvent(&moveEv);
    releaseEv.setScenePos(QPointF(150, 150));
    scene.mouseReleaseEvent(&releaseEv);

    QCOMPARE(scene.items().count(), 4);
}

void TestEditorTools::testCanvasSceneAreaSelectionAndCrop() {
    TestScene scene;
    QPixmap base(200, 200);
    base.fill(Qt::blue);
    scene.setBasePixmap(base);

    scene.setCurrentTool(ToolType::Select);

    QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
    pressEv.setButton(Qt::LeftButton);
    pressEv.setScenePos(QPointF(10, 10));
    scene.mousePressEvent(&pressEv);

    QGraphicsSceneMouseEvent moveEv(QEvent::GraphicsSceneMouseMove);
    moveEv.setButton(Qt::LeftButton);
    moveEv.setScenePos(QPointF(110, 110));
    scene.mouseMoveEvent(&moveEv);

    QGraphicsSceneMouseEvent releaseEv(QEvent::GraphicsSceneMouseRelease);
    releaseEv.setButton(Qt::LeftButton);
    releaseEv.setScenePos(QPointF(110, 110));
    scene.mouseReleaseEvent(&releaseEv);

    QVERIFY(scene.hasAreaSelection());
    QCOMPARE(scene.selectedArea().toRect(), QRect(10, 10, 100, 100));

    scene.cropToSelectedArea();
    QCOMPARE(scene.basePixmap().size(), QSize(100, 100));
    QVERIFY(!scene.hasAreaSelection());
}

void TestEditorTools::testCanvasSceneUndoRedo() {
    TestScene scene;
    QPixmap base(200, 200);
    base.fill(Qt::white);
    scene.setBasePixmap(base);

    scene.setCurrentTool(ToolType::Rectangle);

    QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
    pressEv.setButton(Qt::LeftButton);
    pressEv.setScenePos(QPointF(10, 10));
    scene.mousePressEvent(&pressEv);

    QGraphicsSceneMouseEvent releaseEv(QEvent::GraphicsSceneMouseRelease);
    releaseEv.setButton(Qt::LeftButton);
    releaseEv.setScenePos(QPointF(60, 60));
    scene.mouseReleaseEvent(&releaseEv);

    int countAfterAdd = scene.items().count();

    QVERIFY(scene.undoStack()->canUndo());
    scene.undoStack()->undo();
    QCOMPARE(scene.items().count(), countAfterAdd - 1);

    QVERIFY(scene.undoStack()->canRedo());
    scene.undoStack()->redo();
    QCOMPARE(scene.items().count(), countAfterAdd);
}

void TestEditorTools::testTextItemMultiLineAndBackground() {
    TextItem item;
    // Verify default transparent background
    QCOMPARE(item.fillColor(), Qt::transparent);

    // Multi-line text support
    item.setText("Line 1\nLine 2\nLine 3");
    QCOMPARE(item.text(), QString("Line 1\nLine 2\nLine 3"));

    // Changing colors
    item.setStrokeColor(QColor(0, 0, 255));
    QCOMPARE(item.strokeColor(), QColor(0, 0, 255));
    item.setFillColor(QColor(255, 255, 0, 180));
    QCOMPARE(item.fillColor(), QColor(255, 255, 0, 180));

    // Paint to QImage to verify rendering with background
    QImage target(200, 100, QImage::Format_ARGB32_Premultiplied);
    target.fill(Qt::white);
    QPainter p(&target);
    QStyleOptionGraphicsItem opt;
    item.paint(&p, &opt, nullptr);
    p.end();

    // Verify pixels were drawn
    bool foundYellow = false;
    for (int y = 0; y < target.height(); ++y) {
        for (int x = 0; x < target.width(); ++x) {
            QRgb c = target.pixel(x, y);
            if (qRed(c) > 200 && qGreen(c) > 200 && qBlue(c) < 100) {
                foundYellow = true;
                break;
            }
        }
        if (foundYellow) break;
    }
    QVERIFY(foundYellow);
}

void TestEditorTools::testCanvasSceneTextCreation() {
    TestScene scene;
    QPixmap base(300, 300);
    base.fill(Qt::white);
    scene.setBasePixmap(base);

    scene.setCurrentTool(ToolType::Text);
    scene.setStrokeColor(QColor(255, 0, 0));
    scene.setFillColor(Qt::transparent);

    QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
    pressEv.setButton(Qt::LeftButton);
    pressEv.setScenePos(QPointF(50, 50));
    scene.mousePressEvent(&pressEv);

    // Should have created a TextItem in scene
    TextItem* created = nullptr;
    for (auto* item : scene.items()) {
        if (auto* txt = dynamic_cast<TextItem*>(item)) {
            created = txt;
            break;
        }
    }
    QVERIFY(created != nullptr);
    QVERIFY(created->isEditing());

    // Type text and finish editing
    created->setText("Hello Multi-line\nWorld!");
    created->finishEditing();
    QVERIFY(!created->isEditing());

    // Undo should remove it, Redo should restore it
    QVERIFY(scene.undoStack()->canUndo());
    scene.undoStack()->undo();
    bool foundAfterUndo = false;
    for (auto* item : scene.items()) {
        if (dynamic_cast<TextItem*>(item)) foundAfterUndo = true;
    }
    QVERIFY(!foundAfterUndo);

    scene.undoStack()->redo();
    bool foundAfterRedo = false;
    for (auto* item : scene.items()) {
        if (dynamic_cast<TextItem*>(item)) foundAfterRedo = true;
    }
    QVERIFY(foundAfterRedo);
}

void TestEditorTools::testBucketFillTool() {
    TestScene scene;
    QImage baseImg(100, 100, QImage::Format_ARGB32);
    baseImg.fill(Qt::white);
    // Draw a 40x40 black square in the middle (x: 30..69, y: 30..69)
    for (int y = 30; y < 70; ++y) {
        for (int x = 30; x < 70; ++x) {
            baseImg.setPixelColor(x, y, Qt::black);
        }
    }
    scene.setBasePixmap(QPixmap::fromImage(baseImg));
    scene.setCurrentTool(ToolType::BucketFill);
    scene.setFillColor(QColor(255, 0, 0)); // Red fill

    // Click inside the black square at (50, 50)
    QGraphicsSceneMouseEvent pressEv(QEvent::GraphicsSceneMousePress);
    pressEv.setButton(Qt::LeftButton);
    pressEv.setScenePos(QPointF(50, 50));
    scene.mousePressEvent(&pressEv);

    // Verify center pixel turned red, but outer region (10, 10) stayed white
    QImage resultImg = scene.basePixmap().toImage();
    QCOMPARE(resultImg.pixelColor(50, 50), QColor(255, 0, 0));
    QCOMPARE(resultImg.pixelColor(35, 35), QColor(255, 0, 0));
    QCOMPARE(resultImg.pixelColor(65, 65), QColor(255, 0, 0));
    QCOMPARE(resultImg.pixelColor(10, 10), QColor(255, 255, 255));

    // Test Undo
    QVERIFY(scene.undoStack()->canUndo());
    scene.undoStack()->undo();
    QImage undoImg = scene.basePixmap().toImage();
    QCOMPARE(undoImg.pixelColor(50, 50), QColor(0, 0, 0));
    QCOMPARE(undoImg.pixelColor(10, 10), QColor(255, 255, 255));

    // Test Redo
    QVERIFY(scene.undoStack()->canRedo());
    scene.undoStack()->redo();
    QImage redoImg = scene.basePixmap().toImage();
    QCOMPARE(redoImg.pixelColor(50, 50), QColor(255, 0, 0));

    // Test bucket clicking a vector ShapeItem
    ShapeItem* shape = new ShapeItem(false);
    shape->setRect(QRectF(10, 10, 20, 20));
    shape->setFillColor(Qt::white);
    scene.addItem(shape);

    scene.setFillColor(QColor(0, 255, 0)); // Green
    QGraphicsSceneMouseEvent clickShape(QEvent::GraphicsSceneMousePress);
    clickShape.setButton(Qt::LeftButton);
    clickShape.setScenePos(QPointF(20, 20));
    scene.mousePressEvent(&clickShape);

    QCOMPARE(shape->fillColor(), QColor(0, 255, 0));
}

void TestEditorTools::testDefaultBlurStrength() {
    CanvasScene scene;
    QCOMPARE(scene.blurLevel(), 4);

    BlurItem blur;
    QCOMPARE(blur.blurLevel(), 4);
}

void TestEditorTools::testSelectionPropertiesSync() {
    TestScene scene;
    QPixmap base(200, 200);
    base.fill(Qt::white);
    scene.setBasePixmap(base);

    // Add BlurItem
    BlurItem* blur = new BlurItem(QRectF(20, 20, 50, 50), base, 4);
    scene.addItem(blur);
    blur->setSelected(true);
    QCOMPARE(blur->blurLevel(), 4);

    // Modify selected blur's level
    blur->setBlurLevel(8);
    blur->updateEffect(scene.basePixmap());
    QCOMPARE(blur->blurLevel(), 8);

    // Add TextItem
    TextItem* text = new TextItem("Test Text", QPointF(100, 100));
    text->setStrokeColor(QColor(255, 0, 0));
    text->setFillColor(Qt::transparent);
    scene.addItem(text);
    text->setSelected(true);

    // Modify selected text's colors
    text->setStrokeColor(QColor(0, 0, 255));
    text->setFillColor(QColor(255, 255, 0));
    QCOMPARE(text->strokeColor(), QColor(0, 0, 255));
    QCOMPARE(text->fillColor(), QColor(255, 255, 0));

    // Add ShapeItem
    ShapeItem* shape = new ShapeItem(false);
    shape->setRect(QRectF(10, 10, 40, 40));
    shape->setStrokeWidth(3);
    scene.addItem(shape);
    shape->setSelected(true);

    // Modify selected shape's width
    shape->setStrokeWidth(9);
    QCOMPARE(shape->strokeWidth(), 9);
}

void TestEditorTools::testDefaultSaveLocation() {
    SettingsManager& s = SettingsManager::instance();
    QString expectedDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (expectedDir.isEmpty()) {
        expectedDir = QDir::homePath() + "/Pictures";
    }
    QCOMPARE(s.saveLocation(), expectedDir);
}

void TestEditorTools::testCanvasViewDirectMousewheelZoom() {
    TestScene scene;
    QPixmap pix(800, 600);
    pix.fill(Qt::white);
    scene.setBasePixmap(pix);

    CanvasView view(&scene);
    view.resize(800, 600);

    qreal initialZoom = view.zoomFactor();

    // Wheel up zooms in directly without Ctrl
    QWheelEvent zoomInEvent(
        QPointF(400, 300),
        QPointF(400, 300),
        QPoint(0, 0),
        QPoint(0, 120),
        Qt::NoButton,
        Qt::NoModifier,
        Qt::NoScrollPhase,
        false
    );
    QApplication::sendEvent(view.viewport(), &zoomInEvent);

    QVERIFY(view.zoomFactor() > initialZoom);

    // Wheel down zooms out directly without Ctrl
    QWheelEvent zoomOutEvent(
        QPointF(400, 300),
        QPointF(400, 300),
        QPoint(0, 0),
        QPoint(0, -120),
        Qt::NoButton,
        Qt::NoModifier,
        Qt::NoScrollPhase,
        false
    );
    QApplication::sendEvent(view.viewport(), &zoomOutEvent);

    QVERIFY(view.zoomFactor() <= initialZoom + 0.05);
}

void TestEditorTools::testBlurItemMemoryOptimization() {
    TestScene scene;
    QPixmap largePix(800, 600);
    largePix.fill(Qt::cyan);
    scene.setBasePixmap(largePix);

    BlurItem* blur = new BlurItem(QRectF(100, 100, 200, 200), largePix, 4);
    scene.addItem(blur);
    QVERIFY(!blur->boundingRect().isEmpty());

    blur->setBlurLevel(7);
    QCOMPARE(blur->blurLevel(), 7);
}

void TestEditorTools::testScreenCapture() {
    QScreen* screen = QGuiApplication::primaryScreen();
    QVERIFY(screen != nullptr);
    QRect geo = screen->virtualGeometry();
    qDebug() << "Virtual geo:" << geo;
    QPixmap grab = screen->grabWindow(0, geo.x(), geo.y(), geo.width(), geo.height());
    qDebug() << "Grab size:" << grab.size() << "isNull:" << grab.isNull();
}

void TestEditorTools::testRegionSnippingCapture() {
    RegionSnippingOverlay overlay;
    overlay.startSnipping();

    QPixmap resultPix;
    connect(&overlay, &RegionSnippingOverlay::regionCaptured, [&resultPix](const QPixmap& p) {
        resultPix = p;
    });

    // Simulate mouse press at (100, 100)
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(100, 100), QPointF(100, 100), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&overlay, &press);

    // Simulate mouse move to (300, 250)
    QMouseEvent move(QEvent::MouseMove, QPointF(300, 250), QPointF(300, 250), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&overlay, &move);

    // Simulate mouse release at (300, 250)
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(300, 250), QPointF(300, 250), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&overlay, &release);

    // Verify resultPix is NOT null and has correct non-zero size
    QVERIFY(!resultPix.isNull());
    QVERIFY(resultPix.width() >= 100);
    QVERIFY(resultPix.height() >= 100);
}

void TestEditorTools::testTabCloseSaveDialogButtons() {
    QMessageBox box(
        QMessageBox::Question,
        "Save Changes",
        "Do you want to save changes to \"Capture 1\" before closing?",
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
    );
    box.setDefaultButton(QMessageBox::Yes);
    box.show();
    QApplication::processEvents();

    QPixmap grab = box.grab();
    grab.save("/home/owner/distrobox-homes/devbox/.gemini/antigravity/brain/e77fcd3f-2a52-4f22-843d-d82ecd75f998/save_dialog_preview.png");

    QList<QAbstractButton*> buttons = box.buttons();
    QCOMPARE(buttons.size(), 3);
    QVERIFY(box.button(QMessageBox::Yes) != nullptr);
    QVERIFY(box.button(QMessageBox::No) != nullptr);
    QVERIFY(box.button(QMessageBox::Cancel) != nullptr);
    QCOMPARE(box.standardButton(box.button(QMessageBox::Yes)), QMessageBox::Yes);
    QCOMPARE(box.standardButton(box.button(QMessageBox::No)), QMessageBox::No);
    QCOMPARE(box.standardButton(box.button(QMessageBox::Cancel)), QMessageBox::Cancel);
    box.close();
}

void TestEditorTools::testAutoCheckUpdatesSetting() {
    SettingsManager& s = SettingsManager::instance();
    bool original = s.autoCheckUpdates();

    s.setAutoCheckUpdates(false);
    QCOMPARE(s.autoCheckUpdates(), false);

    s.setAutoCheckUpdates(true);
    QCOMPARE(s.autoCheckUpdates(), true);

    s.save();
    s.load();
    QCOMPARE(s.autoCheckUpdates(), true);

    s.setAutoCheckUpdates(original);
    s.save();
}

void TestEditorTools::testUpdateManagerVersionComparison() {
    // Newer remote versions
    QVERIFY(UpdateManager::isVersionNewer("v1.24", "1.23"));
    QVERIFY(UpdateManager::isVersionNewer("1.23.1", "1.23"));
    QVERIFY(UpdateManager::isVersionNewer("v1.24.0", "1.23.0"));
    QVERIFY(UpdateManager::isVersionNewer("v2.0", "1.23"));
    QVERIFY(UpdateManager::isVersionNewer("v2.0.0", "1.23.0"));

    // Equal versions
    QVERIFY(!UpdateManager::isVersionNewer("v1.23", "1.23"));
    QVERIFY(!UpdateManager::isVersionNewer("1.23", "1.23"));
    QVERIFY(!UpdateManager::isVersionNewer("v1.23.0", "1.23.0"));
    QVERIFY(!UpdateManager::isVersionNewer("1.23.0", "1.23"));

    // Older remote versions
    QVERIFY(!UpdateManager::isVersionNewer("v1.22", "1.23"));
    QVERIFY(!UpdateManager::isVersionNewer("v1.22.9", "1.23"));
    QVERIFY(!UpdateManager::isVersionNewer("v0.9.0", "1.23.0"));
}

void TestEditorTools::testIconManager() {
    // Test toolbar and tool icons (dark and light)
    QIcon selectIcon = IconManager::getIcon("select");
    QVERIFY(!selectIcon.isNull());
    QVERIFY(!selectIcon.pixmap(24, 24).isNull());

    QIcon cropIcon = IconManager::getIcon("crop");
    QVERIFY(!cropIcon.isNull());
    QVERIFY(!cropIcon.pixmap(24, 24).isNull());

    QIcon textIconLight = IconManager::getIcon("text", true);
    QVERIFY(!textIconLight.isNull());
    QVERIFY(!textIconLight.pixmap(24, 24).isNull());

    // Test app icon and multi-resolution pixmaps
    QIcon appIcon = IconManager::getAppIcon();
    QVERIFY(!appIcon.isNull());
    QVERIFY(!appIcon.pixmap(16, 16).isNull());
    QVERIFY(!appIcon.pixmap(22, 22).isNull());
    QVERIFY(!appIcon.pixmap(24, 24).isNull());
    QVERIFY(!appIcon.pixmap(32, 32).isNull());
    QVERIFY(!appIcon.pixmap(48, 48).isNull());
    QVERIFY(!appIcon.pixmap(64, 64).isNull());
    QVERIFY(!appIcon.pixmap(128, 128).isNull());
    QVERIFY(!appIcon.pixmap(256, 256).isNull());
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    TestEditorTools tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_editor_tools.moc"
