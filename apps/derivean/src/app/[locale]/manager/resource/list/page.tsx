import {withResourceQuery} from "@derivean/resource";
import {ResourceIcon}      from "@derivean/ui";
import {
    Breadcrumbs,
    HomeIcon,
    Page
}                          from "@use-pico2/ui";

export default function Index() {
    return <Page
        icon={<ResourceIcon/>}
        title={"manager.resource.list"}
        postfix={<Breadcrumbs
            items={{
                "/manager": {
                    type: "link",
                    href: "/manager",
                    icon: <HomeIcon/>,
                },
            }}
        />}
    >
        <withResourceQuery.store.Provider>

        </withResourceQuery.store.Provider>
    </Page>;
}
