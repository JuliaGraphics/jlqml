using QML

# const qml_file = joinpath(pwd(), "code", "qml", "test.qml")

function update_props()
  props["fruits"] = fruits2
end

function main(qml_file, props)
  loadqml(qml_file, props=props)
  exec()
end

mutable struct Fruit
  name::String
  cost::Float64
end

fruits = JuliaItemModel([Fruit("apple", 1.0), Fruit("orange", 2.0)])
fruits2 = JuliaItemModel([Fruit("banana", 1.0), Fruit("pear", 2.0)])

props = JuliaPropertyMap("fruits" => fruits)

@qmlfunction update_props

# main(qml_file, props)


mktempdir() do folder
  path = joinpath(folder, "test.qml")
  write(
    path,
    """
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.julialang

ApplicationWindow {
    id: mainWin
    title: "My Application"
    width: 400
    height: 400
    visible: true
  
    ColumnLayout{
      anchors.fill: parent
  
        Button {
            height: 200
            Layout.fillWidth: true
            text: "Update Fruit"
            onClicked: { 
                Julia.update_props()
                console.log("Value:", props.fruits) 
            }
       }
        ListView {
            model: props.fruits
            Layout.fillHeight: true
            Layout.fillWidth: true
            delegate: Row {
                Text {
                    text: name
                }
            }
        }
    }
}
"""
  )
  main(path, props)
end