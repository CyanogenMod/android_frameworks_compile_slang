/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This file is auto-generated. DO NOT MODIFY!
 * The source RenderScript file: def1.rs
 */
package foo;

import android.renderscript.*;
import android.content.res.Resources;
import android.util.Log;

/**
 * @hide
 */
public class ScriptC_def1 extends ScriptC {
    // Constructor
    public  ScriptC_def1(RenderScript rs, Resources resources, int id) {
        super(rs, resources, id);
    }

    public  ScriptC_def1(RenderScript rs, Resources resources, int id, boolean isRoot) {
        super(rs, resources, id);
    }

    private final static int mExportVarIdx_o8 = 0;
    private ScriptField_DifferentDefinition8.Item mExportVar_o8;
    public void set_o8(ScriptField_DifferentDefinition8.Item v) {
        mExportVar_o8 = v;
        FieldPacker fp = new FieldPacker(4);
        fp.addI32(v.member1.member1);
        setVar(mExportVarIdx_o8, fp);
    }

    public ScriptField_DifferentDefinition8.Item get_o8() {
        return mExportVar_o8;
    }

}

