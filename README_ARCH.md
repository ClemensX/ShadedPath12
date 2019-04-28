# ShadedPath12 Architecture (branch new_engine3)
DirectX 12 Framework and Applications for Oculus Rift

![Alt text](https://g.gravizo.com/svg?https%3A%2F%2Fgithub.com%2FClemensX%2FShadedPath12%2Fedit%2Fnew_engine3%2FREADME_ARCH.md
  digraph G {
    aize ="4,4";
    main [shape=box];
    main -> parse [weight=8];
    parse -> execute;
    main -> init [style=dotted];
    main -> cleanup;
    execute -> { make_string; printf}
    init -> make_string;
    edge [color=red];
    main -> printf [style=bold,label="100 times"];
    make_string [label="make a string"];
    node [shape=box,style=filled,color=".7 .3 1.0"];
    execute -> compare;
  }
)
