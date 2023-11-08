import {withDefaultPipeline as withCoolDefaultPipeline} from "@use-pico/i18n";
import {withRichComponents}                             from "./withRichComponents";

export const withDefaultPipeline = () => withCoolDefaultPipeline({
    rich: {
        component: {
            components: withRichComponents(),
        },
    },
});
