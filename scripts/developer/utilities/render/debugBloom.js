//
//  debugBloom.js
//  developer/utilities/render
//
//  Olivier Prat, created on 09/25/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('bloom.qml');
var window = new OverlayWindow({
    title: 'Bloom',
    source: qml,
    width: 285, 
    height: 170,
});
window.closed.connect(function() { Script.stop(); });