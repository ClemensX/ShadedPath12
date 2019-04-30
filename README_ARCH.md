# ShadedPath12 Architecture (branch new_engine3)
DirectX 12 Framework and Applications for Oculus Rift

![Alt text](https://g.gravizo.com/source/overview?https%3A%2F%2Fraw.githubusercontent.com%2FClemensX%2FShadedPath12%2Fnew_engine3%2FREADME_ARCH.md)

<details> 
<summary></summary>
@startuml component
actor client
node app
database db

db -> app
app -> client
@enduml
</details>

### Activity
```plantuml
@startuml
|Application|
|Pipeline|
|Frame|
start
    |Application|
    :init;
    |Pipeline|
    :configure;
    |Frame|
    :initRender;
    |Application|
    :update;
    :draw;
    |Pipeline|
    :addDrawRequest;
    split
        |Application|
        stop
    split again
        |Pipeline|
        :prepareFrame;
        |Frame|
        :draw;
        |Pipeline|
        :storeImage;
        |Application|
        :processImage;
        stop
    end split
@enduml
```
