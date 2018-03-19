```
                                                                             .,:l;
                                                                   ..,:ldOKNMMMMMx
                                                                 :NMMMMMMMMMMMMMMx                .:dOKXNNNNXK0Oxoc'
                                                                 cMMMMMMMMMMMMMMMx              :KMMMMMMMMMMMMMMMMMN
                                                                 cMMMMMMMMMMMMMMMx            .0MMMMMMMMMMMMMMMMMMMN
                                                               ..lMMMMMMMMMMMMMMMk....        0MMMMMMMMMMMMMMMMMMMMN
                                                              0MMMMMMMMMMMMMMMMMMMMMMMk      cMMMMMMMMMMMMMMMMMMMMMN
                              .',;:;;'.                       0MMMMMMMMMMMMMMMMMMMMMMMk      OMMMMMMMMMMMMMMMMWKOO00
    kMMMMMMMMMMMMMMMMMX   'lONMMMMMMMMMNO:                    0MMMMMMMMMMMMMMMMMMMMMMMk      KMMMMMMMMMMMMMMMO.
    OMMMMMMMMMMMMMMMMMX.cKMMMMMMMMMMMMMMMMNc                  0MMMMMMMMMMMMMMMMMMMMMMMk      0MMMMMMMMMMMMMMM'
    OMMMMMMMMMMMMMMMMMWXMMMMMMMMMMMMMMMMMMMMo                 kNNNMMMMMMMMMMMMMMMWNNNNd    ..KMMMMMMMMMMMMMMM;.....
    OMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMW'                   ,MMMMMMMMMMMMMMMx        oMMMMMMMMMMMMMMMMMMMMMMMW.
    kWWMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMo                   .XWMMMMMMMMMMMMMx        oMMMMMMMMMMMMMMMMMMMMMMMW.
      ,MMMMMMMMMMMMMMMMWkc:oXMMMMMMMMMMMMMMMNc                     .,oKMMMMMMMMMMx        oMMMMMMMMMMMMMMMMMMMMMMMW.
      'MMMMMMMMMMMMMMMM;     0MMMMMMMMMMMMO;                           'xWMMMMMMMx        oMMMMMMMMMMMMMMMMMMMMMMMW.
      'MMMMMMMMMMMMMMMX      dMMMMMMMMMMX,                               .OMMMMMMx        oMMMMMMMMMMMMMMMMMMMMMMMW.
      'MMMMMMMMMMMMMMMX      dMMMMMMMMMO.                                  dMMMMMk        oWWMMMMMMMMMMMMMMMMMWWWWW.
      'MMMMMMMMMMMMMMMX      dMMMMMMMMK                ;lAl;                xMMMMWc.   .     0MMMMMMMMMMMMMMMc
      'MMMMMMMMMMMMMMMX      dMMMMMMMM,              .XMMMMMW;              .NMMMMMMWNWMl    OMMMMMMMMMMMMMMM:
      'MMMMMMMMMMMMMMMX      dMMMMMMMK               kMMMMMMMX               dMMMMMMMMMMl    OMMMMMMMMMMMMMMM:
    cokMMMMMMMMMMMMMMMWoo:   dMMMMMMMk               XMMMMMMMM'              cMMMMMMMMMMl    OMMMMMMMMMMMMMMM:
    NMMMMMMMMMMMMMMMMMMMMK   dMMMMMMM0               0MMMMMMMW.              dMMMMMMMMWK,    OMMMMMMMMMMMMMMM:
    NMMMMMMMMMMMMMMMMMMMMK   dMMMMMMMW'              cMMMMMMMk               oOkkxoc;.       OMMMMMMMMMMMMMMM:
    NMMMMMMMMMMMMMMMMMMMMK   dMMMMMMMMO               oWMMMWx.                               OMMMMMMMMMMMMMMM:
    looooooooooooooooooooc   ,ooooooooo.                ','                                  OMMMMMMMMMMMMMMM:
                                                                                             OMMMMMMMMMMMMMMM:
                                                                                             OMMMMMMMMMMMMMMM:
                                USER INTERFACE ENGINE                                        ;lllllllllllllll.
```

The Application module
======================


Application
    PropertyGraph (1)
    ResourceManager (1)
    ThreadPool (1)
    Window (*)
        RenderManager (1)
        GraphicsContext(1)
        FontManager(1, shared)

        SceneManager (1)
            Layer, direct & buffered (*)
                Producer (1)
                    Scene (1, shared)
                        Item (*)

The Application is a singleton and owns a PropertyGraph as well as all Window.

Windows are actually shared_ptrs because they are controlled by the user. The Application on the other side is not
(only instanciated).
