"use client";

import {HeroIcon}            from "@derivean/ui";
import {t}                   from "@use-pico/translator";
import {
    type ComponentProps,
    type FC
}                            from "react";
import {EventHeroComponents} from "../ui/EventHeroComponents";

export namespace EventHeroUpsertForm {
    export type Props = Omit<ComponentProps<EventHeroComponents["Form"]["Mutation"]>, "inputs" | "defaultValues" | "Render">;
}

export const EventHeroUpsertForm: FC<EventHeroUpsertForm.Props> = (
    {
        values,
        ...props
    }
) => {
    return <EventHeroComponents.Form.Mutation
        text={{
            submit:  values ? t`Update event (label)` : t`Create event (label)`,
            success: {
                title:   t`Success`,
                message: values ? t`EventHero updated` : t`EventHero created`,
            }
        }}
        icon={<HeroIcon/>}
        // inputs={{
        //     amount: props => <NumberInput
        //         label={t`EventHero amount`}
        //         {...props}
        //     />,
        // }}
        // values={entity}
        // toRequest={values => (entity ? {
        //     update: {
        //         update: values,
        //         query:  {
        //             where: {
        //                 id: entity.id,
        //             },
        //         },
        //     },
        // } : {
        //     create: values,
        // })}
        defaultValues={{
            amount:  1,
            eventId: "",
        }}
        // Render={({Input}) => <>
        //     <Input name={"amount"}/>
        // </>}
        {...props}
    >
        {/*<Input */}
    </EventHeroComponents.Form.Mutation>
};
