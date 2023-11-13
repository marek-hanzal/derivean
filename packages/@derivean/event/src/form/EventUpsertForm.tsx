import {EventIcon} from "@derivean/ui";
import {TextInput} from "@use-pico/form";
import {
    t,
    tx
}                  from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                  from "react";
import {EventUI}   from "../ui/EventUI";

export namespace EventUpsertForm {
    export type Props = Omit<ComponentProps<EventUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const EventUpsertForm: FC<EventUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <EventUI.MutationForm
        text={{
            submit:  entity ? t()`Update event (label)` : t()`Create event (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Event updated` : t()`Event created`,
            }
        }}
        icon={<EventIcon/>}
        inputs={{
            name: props => <TextInput
                label={t()`Event name`}
                placeholder={tx()`Event name (placeholder)`}
                {...props}
            />,
        }}
        values={entity}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            name: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
        </>}
        {...props}
    />;
};
