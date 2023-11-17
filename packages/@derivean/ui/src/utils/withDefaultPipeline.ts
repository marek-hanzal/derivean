import {withDefaultPipeline as withCoolDefaultPipeline} from "@use-pico/translator";
import {withRichComponents}                             from "./withRichComponents";

export const withDefaultPipeline = () => withCoolDefaultPipeline({
    rich: {
        component: {
            components: withRichComponents(),
        },
    },
});
