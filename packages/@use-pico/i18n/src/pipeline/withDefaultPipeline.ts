import {withDefaultRichPipeline} from "./withDefaultRichPipeline";
import {withDefaultTextPipeline} from "./withDefaultTextPipeline";

export namespace withDefaultPipeline {
    export interface Props {
        text?: withDefaultTextPipeline.Props;
        rich: withDefaultRichPipeline.Props;
    }
}

export const withDefaultPipeline = (
    {
        text = {},
        rich,
    }: withDefaultPipeline.Props,
) => {
    return {
        text: withDefaultTextPipeline(text),
        rich: withDefaultRichPipeline(rich),
    };
};
