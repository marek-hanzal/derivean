import {InterpolatePipeline} from "./InterpolatePipeline";

export namespace withDefaultTextPipeline {
    export interface Props {
    }
}

export const withDefaultTextPipeline = (
    {}: withDefaultTextPipeline.Props,
) => [
    InterpolatePipeline(),
];
