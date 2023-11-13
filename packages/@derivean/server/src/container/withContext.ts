import {withEventContext}   from "@derivean/event";
import {withKingdomContext} from "@derivean/kingdom";
import {type IContainer}    from "@use-pico/container";

const context = [
    withEventContext,
    withKingdomContext,
] as const;

export const withContext: IContainer.Register = container => {
    context.forEach(register => register(container));
};
