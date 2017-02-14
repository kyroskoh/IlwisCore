#ifndef NODE_H
#define NODE_H

#include <QObject>
#include <QQmlListProperty>

class Node : public QObject
{
    Q_OBJECT
    Q_ENUMS(NodeType)
    Q_PROPERTY( QString name READ name WRITE setName NOTIFY nameChanged )
    Q_PROPERTY( QString unit READ unit WRITE setUnit NOTIFY unitChanged )
    Q_PROPERTY( int type READ type NOTIFY typeChanged )
    Q_PROPERTY( double weight READ weight NOTIFY weightChanged )
    Q_PROPERTY( QQmlListProperty<Node> subNodes READ subNodesQml NOTIFY subNodesChanged)
    Q_PROPERTY( QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged )
    Q_PROPERTY( int level READ level NOTIFY levelChanged )

signals:
   void nameChanged();
   void unitChanged();
   void typeChanged();
   void weightChanged();
   void subNodesChanged();
   void fileNameChanged();
   void levelChanged();

public:
    enum NodeType { Group=0, MaskArea=1, Constraint=2, Factor=3 };
    Node();
    Node(QObject *qparent);
    Node(Node * parent, QObject *qparent);
    int type() const;
    void setType(NodeType nt);
    const QString name() const;
    void setName(QString name);
    const QString unit() const;
    void setUnit(QString unit);
    double weight() const;
    void setWeight(double weight);
    const Node * parent() const;
    QList <Node*> subNodes();
    QQmlListProperty<Node> subNodesQml();
    void addNode(Node *node);
    const QString fileName() const;
    void setFileName(QString fileName);
    int level() const;
    void deleteChild(Node * node);
    void recalcWeights();
    Q_INVOKABLE void setGoal(QString name);
    Q_INVOKABLE void addMask(QString name);
    Q_INVOKABLE Node * addGroup(QString name);
    Q_INVOKABLE void addFactor(QString name);
    Q_INVOKABLE void addConstraint(QString name);
    Q_INVOKABLE void deleteNode();

protected:
    NodeType _type;
    QString _name;
    QString _unit;
    Node * _parent;
    double _weight;
    QList <Node*> _subNodes;
    QString _fileName;
};

#endif // NODE_H
